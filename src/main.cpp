/*
********************************************************************************
*                                                                              *
*           WeatherDNA - Stevenson Screen Environmental Node v2.0              *
*           Proudly presented by: Gemini CLI & Silas Mariusz Grzybacz          *
*                                                                              *
********************************************************************************
*/

#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <esp_wifi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <ESPmDNS.h>
#include <esp_sleep.h>
#include <ArduinoOTA.h>
#include <esp_task_wdt.h>
#include <Update.h>
#include <math.h>
#include <PubSubClient.h>
#include <vector>
#include <Adafruit_NeoPixel.h>
#include "driver/pcnt.h"
#include "secrets.h"

// --- SENSOR LIBRARIES ---
#include <bsec2.h>
#include "D:/Arduino/libraries/bsec_v3-3-0-0/release_bin/IAQ/config/bme690/bme690_iaq_33v_3s_4d/bsec_iaq.h"
#include "SparkFun_BMV080_Arduino_Library.h"
#include "Adafruit_SHT4x.h"
#include <SensirionI2CSgp41.h>
#include <VOCGasIndexAlgorithm.h>
#include <NOxGasIndexAlgorithm.h>
#include <SensirionI2cScd4x.h>
#include <Adafruit_BMP5xx.h>
#include <ILPS22QSSensor.h>
#include "DFRobot_AS3935_I2C.h"
#include "no2_o3-arduino.h"
#include "hal/arduino/arduino_hal.h"

// --- GLOBAL CONFIG ---
#define DEBUG true
#define ERROR_LED true
#define RGB_LED_PIN 48
#define I2C_SDA_PIN 4
#define I2C_SCL_PIN 5
#define RAIN_RG15_RX 44
#define RAIN_RG15_TX 43
#define AS3935_IRQ_PIN 7
#define HOSTNAME "WeatherDNA-Stevenson"
#define MQTT_TOPIC_STATE "weatherdna/state"

// --- LOGGING LEVELS ---
enum LogLevel { LOG_INFO=0, LOG_WARN=1, LOG_ERROR=2, LOG_CORRUPTED=3, LOG_ANOMALY=4, LOG_SENSOR_FAILED=5, LOG_BOOT_FROM_CRASH=6 };

// --- SYSTEM MODES ---
enum SystemMode { MODE_CONTINUOUS, MODE_DEEP_SLEEP, MODE_LIGHT_SLEEP, MODE_MAINTENANCE, MODE_RECOVERY, MODE_DEV };
enum CyclePhase { PHASE_WAKEUP, PHASE_WARMUP, PHASE_READ, PHASE_PUSH_API, PHASE_SLEEP_WAIT };

// --- FORWARD DECLARATIONS ---
void ATLAS_LOG(LogLevel level, bool verbose, const char* format, ...);
bool tcaselect(uint8_t mux_addr, uint8_t i);
bool repairWiFi();
void checkRemoteModeOverride();
void fetchWindFromWU();
void pushDataAPI();
void pushWeatherUnderground();
void pushAwekas();
void pushWeathercloud();
void pushMQTT();
float calcSLP(float p, float t, float alt);

// --- GLOBALS ---
SystemMode currentMode = MODE_CONTINUOUS;
CyclePhase currentPhase = PHASE_WAKEUP;
Adafruit_NeoPixel statusLed(1, RGB_LED_PIN, NEO_GRB + NEO_KHZ800);
WiFiClient espClient;
PubSubClient mqtt(espClient);
JsonDocument payload;
Preferences prefs;
WebServer server(80);

// Sensor Objects
Bsec2 bme688;
SparkFunBMV080 bmv080;
Adafruit_SHT4x sht45;
SensirionI2CSgp41 sgp41;
VOCGasIndexAlgorithm vocAlgorithm;
NOxGasIndexAlgorithm noxAlgorithm;
SensirionI2cScd4x scd41;
Adafruit_BMP5xx bmp585;
ILPS22QSSensor ilps(&Wire);
DFRobot_AS3935_I2C lightning(AS3935_IRQ_PIN, 0x03);

// Sensor Status Flags
bool bme688_ok = false, zmod4510_ok = false, scd41_ok = false, sgp41_ok = false;
bool bmv080_ok = false, sht45_ok = false, ilps_ok = false, bmp585_ok = false;
bool rg15_ok = false, as3935_ok = false, i2c_mem_ok = false;

// ZMOD4510 Algos
static zmod4xxx_dev_t zmod4510_dev;
static uint8_t zmod4510_adc[ZMOD4510_ADC_DATA_LEN];
static uint8_t zmod4510_prod[ZMOD4510_PROD_DATA_LEN];
static no2_o3_handle_t zmod4510_algo_handle;
static no2_o3_results_t zmod4510_results;
static no2_o3_inputs_t zmod4510_input;
static Interface_t zmod4510_hal;

// Global Buffers for Indices
int32_t current_voc_index = 0, current_nox_index = 0;
uint16_t current_sraw_voc = 0, current_sraw_nox = 0;

// Wind Workaround Data
float wind_speed = 0.0f, wind_dir = 0.0f, wind_gust = 0.0f;
bool wind_fetched = false;

// Timers
unsigned long cycleStartTime = 0;
unsigned long dynamic_warmup_ms = 45000;
unsigned long lastBsecPollTime = 0;
unsigned long lastFastPollTime = 0;
bool scd41_triggered = false;

// MUX addresses for Stevenson Screen
const uint8_t MUX_ADDR = 0x72;

// LED Task Data
struct LedPattern {
    uint32_t color;
    int count;
    int duration;
    int pause;
};

// -----------------------------------------------------------------------
// 1. ATLAS_LOG WRAPPER & LED FEEDBACK
// -----------------------------------------------------------------------

void setLed(uint32_t color, uint8_t brightness) {
    if (!ERROR_LED) return;
    statusLed.setPixelColor(0, color);
    statusLed.setBrightness(brightness);
    statusLed.show();
}

void blinkLedTask(void* parameter) {
    LedPattern* pattern = (LedPattern*)parameter;
    for (int i = 0; i < pattern->count; i++) {
        setLed(pattern->color, 255);
        vTaskDelay(pdMS_TO_TICKS(pattern->duration));
        setLed(0, 0);
        vTaskDelay(pdMS_TO_TICKS(pattern->pause));
    }
    delete pattern;
    vTaskDelete(NULL);
}

void triggerLedPattern(LogLevel level) {
    if (!ERROR_LED) return;
    LedPattern* pattern = new LedPattern();
    switch (level) {
        case LOG_INFO: // Green 3%
            pattern->color = statusLed.Color(0, 8, 0); pattern->count = 1; pattern->duration = 200; pattern->pause = 0;
            break;
        case LOG_WARN: // Orange 10%
            pattern->color = statusLed.Color(25, 16, 0); pattern->count = 1; pattern->duration = 300; pattern->pause = 0;
            break;
        case LOG_ERROR: // Red 100%
            pattern->color = statusLed.Color(255, 0, 0); pattern->count = 1; pattern->duration = 1000; pattern->pause = 0;
            break;
        case LOG_CORRUPTED: // 2x Pink 10%
            pattern->color = statusLed.Color(25, 0, 25); pattern->count = 2; pattern->duration = 100; pattern->pause = 100;
            break;
        case LOG_ANOMALY: // 3x Orange 10%
            pattern->color = statusLed.Color(25, 16, 0); pattern->count = 3; pattern->duration = 80; pattern->pause = 80;
            break;
        case LOG_SENSOR_FAILED: // 1x Long Violet 15%
            pattern->color = statusLed.Color(38, 0, 38); pattern->count = 1; pattern->duration = 800; pattern->pause = 0;
            break;
        case LOG_BOOT_FROM_CRASH: // 7x White 10%
            pattern->color = statusLed.Color(25, 25, 25); pattern->count = 7; pattern->duration = 100; pattern->pause = 100;
            break;
    }
    xTaskCreate(blinkLedTask, "led_task", 2048, pattern, 1, NULL);
}

void ATLAS_LOG(LogLevel level, bool verbose, const char* format, ...) {
    char buf[1024];
    va_list arg;
    va_start(arg, format);
    vsnprintf(buf, sizeof(buf), format, arg);
    va_end(arg);

    triggerLedPattern(level);

    if (DEBUG || verbose || level >= LOG_WARN) {
        Serial.print(buf);
    }
}

// -----------------------------------------------------------------------
// 2. I2C MUX WRAPPER & TOPOLOGY
// -----------------------------------------------------------------------

bool tcaselect(uint8_t mux_addr, uint8_t i) {
    if (i > 7) return false;
    Wire.beginTransmission(mux_addr);
    Wire.write(1 << i);
    bool ok = (Wire.endTransmission() == 0);
    if (!ok) ATLAS_LOG(LOG_SENSOR_FAILED, false, "[I2C] Failed to select CH%d on MUX 0x%02X\n", i, mux_addr);
    delay(5);
    return ok;
}

float calcSLP(float p, float t, float alt) {
    return p * pow((1.0f - (0.0065f * alt) / (t + 0.0065f * alt + 273.15f)), -5.257f);
}

// -----------------------------------------------------------------------
// 3. SENSOR INITIALIZATION
// -----------------------------------------------------------------------

bool initBME688() {
    if(!tcaselect(MUX_ADDR, 0)) return false;
    if (bme688.begin(BME68X_I2C_ADDR_HIGH, Wire)) {
        bme688.setConfig(bsec_config_iaq);
        bsecSensor sList[] = { BSEC_OUTPUT_IAQ, BSEC_OUTPUT_CO2_EQUIVALENT, BSEC_OUTPUT_BREATH_VOC_EQUIVALENT };
        bme688.updateSubscription(sList, 3, BSEC_SAMPLE_RATE_LP);
        bme688_ok = true;
        ATLAS_LOG(LOG_INFO, true, " |- CH0: BME688 AI -> ONLINE\n");
        return true;
    }
    return false;
}

bool initZMOD4510() {
    if (!tcaselect(MUX_ADDR, 1)) return false;
    HAL_Init(&zmod4510_hal);
    zmod4510_dev.i2c_addr = ZMOD4510_I2C_ADDR;
    zmod4510_dev.pid = ZMOD4510_PID;
    zmod4510_dev.init_conf = &zmod_no2_o3_sensor_cfg[INIT];
    zmod4510_dev.meas_conf = &zmod_no2_o3_sensor_cfg[MEASUREMENT];
    zmod4510_dev.prod_data = zmod4510_prod;
    if (zmod4xxx_init(&zmod4510_dev, &zmod4510_hal) == 0) {
        init_no2_o3(&zmod4510_algo_handle);
        zmod4510_ok = true;
        ATLAS_LOG(LOG_INFO, true, " |- CH1: ZMOD4510 -> ONLINE\n");
        return true;
    }
    return false;
}

void initSensors() {
    initBME688();
    initZMOD4510();
    
    if(tcaselect(MUX_ADDR, 2)) {
        scd41.begin(Wire, 0x62); sgp41.begin(Wire);
        scd41_ok = sgp41_ok = true;
        ATLAS_LOG(LOG_INFO, true, " |- CH2: SCD41+SGP41 -> ONLINE\n");
    }
    if(tcaselect(MUX_ADDR, 3)) {
        if(bmv080.begin()) { bmv080.init(); bmv080.setMode(1); bmv080_ok = true; ATLAS_LOG(LOG_INFO, true, " |- CH3: BMV080 -> ONLINE\n"); }
    }
    if(tcaselect(MUX_ADDR, 5)) {
        if(sht45.begin()) { sht45_ok = true; ATLAS_LOG(LOG_INFO, true, " |- CH5: SHT45 -> ONLINE\n"); }
    }
    if(tcaselect(MUX_ADDR, 6)) {
        if(ilps.begin() == 0) { ilps.Enable(); ilps_ok = true; ATLAS_LOG(LOG_INFO, true, " |- CH6: ILPS22QS -> ONLINE\n"); }
    }
    if(tcaselect(MUX_ADDR, 7)) {
        if(bmp585.begin()) { bmp585_ok = true; ATLAS_LOG(LOG_INFO, true, " |- CH7: BMP585 -> ONLINE\n"); }
    }
}

// -----------------------------------------------------------------------
// 4. DATA PUSHING & MQTT
// -----------------------------------------------------------------------

void pushMQTT() {
    if (!mqtt.connected()) {
        mqtt.setServer(MQTT_SERVER, 1883);
        mqtt.connect(HOSTNAME, MQTT_USER, MQTT_PASS);
    }
    if (mqtt.connected()) {
        String out; serializeJson(payload, out);
        mqtt.publish(MQTT_TOPIC_STATE, out.c_str());
        ATLAS_LOG(LOG_INFO, false, "[MQTT] Published payload\n");
    }
}

void pushDataAPI() {
    if (!repairWiFi()) return;
    WiFiClientSecure client; client.setInsecure();
    HTTPClient http; http.begin(client, API_URL);
    http.addHeader("Content-Type", "application/json");
    String jsonStr; serializeJson(payload, jsonStr);
    int code = http.POST(jsonStr);
    ATLAS_LOG(LOG_INFO, true, "[API] HTTP %d\n", code);
    http.end();
}

void fetchWindFromWU() {
    if (!repairWiFi()) return;
    String url = "http://api.weather.com/v2/pws/observations/current?stationId=" + String(WU_STATION_ID) + "&format=json&units=m&apiKey=e1f10a1e78194ce3b10a1e7819ece351";
    HTTPClient http; http.begin(url);
    if (http.GET() == 200) {
        JsonDocument doc; deserializeJson(doc, http.getString());
        wind_speed = doc["observations"][0]["metric"]["windSpeed"] | 0.0f;
        wind_dir = doc["observations"][0]["winddir"] | 0.0f;
        wind_gust = doc["observations"][0]["metric"]["windGust"] | 0.0f;
        payload["sensors"]["WIND_Speed_Kph"] = wind_speed;
        payload["sensors"]["WIND_Direction_Deg"] = wind_dir;
        wind_fetched = true;
    }
    http.end();
}

void pushWeatherUnderground() {
    if (!repairWiFi()) return;
    float tempC = payload["sensors"]["SHT45_Temp"] | 20.0f;
    float pressHpa = payload["sensors"]["METEO_Sea_Level_Press_hPa"] | 1013.25f;
    String url = "http://rtupdate.wunderground.com/weatherstation/updateweatherstation.php?ID=" + String(WU_STATION_ID) + "&PASSWORD=" + String(WU_STATION_KEY) + "&dateutc=now&tempf=" + String((tempC*1.8f)+32.0f,1) + "&baromin=" + String(pressHpa*0.02953f,2) + "&action=updateraw";
    if(wind_fetched) url += "&windspdmph=" + String(wind_speed*0.621f,1) + "&winddir=" + String((int)wind_dir);
    HTTPClient http; http.begin(url); http.GET(); http.end();
}

void pushAwekas() {
    if (!repairWiFi()) return;
    float tempC = payload["sensors"]["SHT45_Temp"] | 20.0f;
    String url = "http://ws.awekas.at/weatherstation/updateweatherstation.php?ID=" + String(AWEKAS_USER) + "&PASSWORD=" + String(AWEKAS_PASS) + "&tempf=" + String((tempC*1.8f)+32.0f,1) + "&action=updateraw";
    HTTPClient http; http.begin(url); http.GET(); http.end();
}

// -----------------------------------------------------------------------
// 5. DIAGNOSTICS & MENU
// -----------------------------------------------------------------------

void i2cScanner() {
    ATLAS_LOG(LOG_INFO, true, "\n--- I2C SCANNER ---\n");
    for (uint8_t addr = 1; addr < 127; addr++) {
        Wire.beginTransmission(addr);
        if (Wire.endTransmission() == 0) Serial.printf("[0x%02X] ", addr);
    }
    Serial.println();
}

void handleDiagnosticMenu() {
    if (Serial.available()) {
        char c = Serial.read();
        if (c == 's') i2cScanner();
        if (c == 'm') currentMode = MODE_CONTINUOUS;
    }
}

// -----------------------------------------------------------------------
// 6. MAIN LOOP
// -----------------------------------------------------------------------

void setup() {
    Serial.begin(115200);
    statusLed.begin(); setLed(0, 0);
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN); Wire.setTimeOut(200);
    WiFi.begin(WIFI_SSIDS[0], WIFI_PASSWORD);
    initSensors();
    cycleStartTime = millis();
}

void loop() {
    esp_task_wdt_reset();
    if (currentMode == MODE_MAINTENANCE) handleDiagnosticMenu();

    switch (currentPhase) {
        case PHASE_WAKEUP:
            payload.clear();
            currentPhase = PHASE_WARMUP;
            break;
        case PHASE_WARMUP:
            if (millis() - lastBsecPollTime > 100) { bme688.run(); lastBsecPollTime = millis(); }
            if (millis() - cycleStartTime > dynamic_warmup_ms) currentPhase = PHASE_READ;
            break;
        case PHASE_READ:
            fetchWindFromWU();
            if(sht45_ok && tcaselect(MUX_ADDR, 5)) { sensors_event_t h, t; sht45.getEvent(&h, &t); payload["sensors"]["SHT45_Temp"] = t.temperature; payload["sensors"]["SHT45_Hum"] = h.relative_humidity; }
            if(bmp585_ok && tcaselect(MUX_ADDR, 7)) { if(bmp585.performReading()) { float p = bmp585.pressure/100.0f; payload["sensors"]["METEO_Sea_Level_Press_hPa"] = calcSLP(p, payload["sensors"]["SHT45_Temp"]|20.0f, 290.0f); } }
            currentPhase = PHASE_PUSH_API;
            break;
        case PHASE_PUSH_API:
            pushMQTT(); pushDataAPI(); pushWeatherUnderground(); pushAwekas();
            currentPhase = PHASE_SLEEP_WAIT;
            break;
        case PHASE_SLEEP_WAIT:
            delay(10000);
            cycleStartTime = millis();
            currentPhase = PHASE_WAKEUP;
            break;
    }
}


