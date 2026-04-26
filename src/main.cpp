/*
********************************************************************************
*                                                                              *
*           WeatherDNA - ATLAS Environmental OS v2.0                           *
*           Stevenson Screen & Optical Dome Integrated Node                    *
*                                                                              *
********************************************************************************
*/

#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <ArduinoOTA.h>
#include <esp_task_wdt.h>
#include <PubSubClient.h>
#include <Adafruit_NeoPixel.h>
#include "driver/pcnt.h"

#include "config.h"
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
#include "DFRobot_BMM350.h"
#include <Adafruit_LSM6DS33.h> 
#include <Adafruit_LIS3MDL.h>
#include <SparkFun_AS7343.h>
#include <SparkFun_AS7331.h>
#include "Adafruit_VEML7700.h"
#include "no2_o3-arduino.h"
#include "hal/arduino/arduino_hal.h"

// --- TYPES & ENUMS ---
enum LogLevel { L_INFO=0, L_WARN=1, L_ERROR=2, L_CORRUPTED=3, L_ANOMALY=4, L_SENSOR_FAILED=5, L_BOOT_CRASH=6 };
enum SystemMode { M_CONTINUOUS, M_DEEP_SLEEP, M_LIGHT_SLEEP, M_MAINTENANCE, M_DEV };
enum CyclePhase { P_WAKEUP, P_WARMUP, P_READ, P_PUSH, P_WAIT };

// --- GLOBALS ---
SystemMode currentMode = M_CONTINUOUS;
CyclePhase currentPhase = P_WAKEUP;
Adafruit_NeoPixel statusLed(1, RGB_LED_PIN, NEO_GRB + NEO_KHZ800);
WiFiClient espClient;
PubSubClient mqtt(espClient);
JsonDocument payload;
Preferences prefs;
unsigned long cycleStartTime = 0;
unsigned long lastBsecPoll = 0;
float wind_spd=0, wind_deg=0, wind_gst=0;
bool wind_ready=false;

// Objects
Bsec2 bme688;
SparkFunBMV080 bmv080;
Adafruit_SHT4x sht45;
SensirionI2cScd4x scd41;
SensirionI2CSgp41 sgp41;
Adafruit_BMP5xx bmp585;
ILPS22QSSensor ilps(&Wire);
DFRobot_AS3935_I2C lightning(AS3935_IRQ_PIN, 0x03);
DFRobot_BMM350_I2C bmm350(&Wire);
Adafruit_VEML7700 veml;
VOCGasIndexAlgorithm vocAlgo;
NOxGasIndexAlgorithm noxAlgo;

// ZMOD4510
static zmod4xxx_dev_t zmod_dev;
static uint8_t zmod_adc[ZMOD4510_ADC_DATA_LEN], zmod_prod[ZMOD4510_PROD_DATA_LEN];
static no2_o3_handle_t zmod_algo;
static no2_o3_results_t zmod_res;
static no2_o3_inputs_t zmod_in;
static Interface_t zmod_hal;

// Flags
bool bme_ok=0, zmod_ok=0, bmv_ok=0, scd_ok=0, sgp_ok=0, sht_ok=0, bmp_ok=0, ilps_ok=0, bmm_ok=0, as3935_ok=0, veml_ok=0;
volatile uint32_t geiger_pulses = 0;

// -----------------------------------------------------------------------
// 1. ASYNC LOGGING & LED SYSTEM
// -----------------------------------------------------------------------

struct LedPattern { uint32_t color; int count; int dur; int gap; };
void ledTask(void* param) {
    LedPattern* p = (LedPattern*)param;
    for (int i=0; i < p->count; i++) {
        statusLed.setPixelColor(0, p->color); statusLed.show();
        vTaskDelay(pdMS_TO_TICKS(p->dur));
        statusLed.setPixelColor(0, 0); statusLed.show();
        vTaskDelay(pdMS_TO_TICKS(p->gap));
    }
    delete p; vTaskDelete(NULL);
}

void triggerLed(LogLevel level) {
    if (!ERROR_LED_ENABLED) return;
    LedPattern* p = new LedPattern();
    switch(level) {
        case L_INFO:       p->color = statusLed.Color(0,8,0);   p->count=1; p->dur=150; p->gap=0; break;
        case L_WARN:       p->color = statusLed.Color(30,15,0); p->count=1; p->dur=300; p->gap=0; break;
        case L_ERROR:      p->color = statusLed.Color(255,0,0); p->count=1; p->dur=1000; p->gap=0; break;
        case L_CORRUPTED:  p->color = statusLed.Color(30,0,30); p->count=2; p->dur=100; p->gap=100; break;
        case L_ANOMALY:    p->color = statusLed.Color(30,15,0); p->count=3; p->dur=80;  p->gap=80; break;
        case L_SENSOR_FAILED: p->color = statusLed.Color(40,0,40); p->count=1; p->dur=800; p->gap=0; break;
        case L_BOOT_CRASH: p->color = statusLed.Color(30,30,30); p->count=7; p->dur=100; p->gap=100; break;
    }
    xTaskCreate(ledTask, "led", 2048, p, 1, NULL);
}

void ATLAS_LOG(LogLevel level, bool verbose, const char* fmt, ...) {
    char buf[512]; va_list args; va_start(args, fmt); vsnprintf(buf, sizeof(buf), fmt, args); va_end(args);
    triggerLed(level);
    if (DEBUG_LEVEL >= 2 || verbose || level >= L_WARN) Serial.print(buf);
}

// -----------------------------------------------------------------------
// 2. I2C MUX WRAPPER
// -----------------------------------------------------------------------

bool muxSelect(uint8_t mux, uint8_t ch) {
    if (ch > 7) return false;
    if (mux != MUX_POWER)   { Wire.beginTransmission(MUX_POWER);   Wire.write(0); Wire.endTransmission(); }
    if (mux != MUX_OPTICS)  { Wire.beginTransmission(MUX_OPTICS);  Wire.write(0); Wire.endTransmission(); }
    if (mux != MUX_WEATHER) { Wire.beginTransmission(MUX_WEATHER); Wire.write(0); Wire.endTransmission(); }
    Wire.beginTransmission(mux); Wire.write(1 << ch);
    return (Wire.endTransmission() == 0);
}

float calcSLP(float p, float t) {
    return p * pow((1.0f - (0.0065f * STATION_ALTITUDE) / (t + 0.0065f * STATION_ALTITUDE + 273.15f)), -5.257f);
}

// -----------------------------------------------------------------------
// 3. DIAGNOSTICS & MENU
// -----------------------------------------------------------------------

void i2cScanner() {
    ATLAS_LOG(L_INFO, true, "\n--- HUMAN-READABLE I2C SCANNER ---\n");
    static std::vector<uint8_t> last_found;
    while(!Serial.available()) {
        std::vector<uint8_t> current;
        for(uint8_t addr=1; addr<127; addr++) {
            Wire.beginTransmission(addr);
            if(Wire.endTransmission() == 0) {
                current.push_back(addr);
                bool is_new = true; for(uint8_t o : last_found) if(o==addr) is_new=false;
                String name = "Unknown";
                if(addr==0x72) name="MUX Stevenson"; else if(addr==0x77) name="BME688/690";
                else if(addr==0x33) name="ZMOD4510"; else if(addr==0x62) name="SCD41";
                else if(addr==0x59) name="SGP41"; else if(addr==0x57) name="BMV080";
                else if(addr==0x44) name="SHT45"; else if(addr==0x5C) name="ILPS22QS";
                else if(addr==0x46) name="BMP585"; else if(addr==0x50) name="EEPROM";
                Serial.printf("[0x%02X: %s%s] ", addr, name.c_str(), is_new?"*":"");
            }
        }
        Serial.println(); last_found = current; delay(2000);
    }
    Serial.read();
}

void handleDiagnosticMenu() {
    if(Serial.available()) {
        char c = Serial.read();
        if(c=='s') i2cScanner();
        if(c=='m') { currentMode = M_CONTINUOUS; ATLAS_LOG(L_INFO, true, "Mode: CONTINUOUS\n"); }
    }
}

// -----------------------------------------------------------------------
// 4. CLOUD & WIND WORKAROUND
// -----------------------------------------------------------------------

void fetchWind() {
    if(WiFi.status() != WL_CONNECTED) return;
    HTTPClient http;
    String url = "http://api.weather.com/v2/pws/observations/current?stationId=" + String(WU_STATION_ID) + "&format=json&units=m&apiKey=e1f10a1e78194ce3b10a1e7819ece351";
    http.begin(url);
    if(http.GET() == 200) {
        JsonDocument d; deserializeJson(d, http.getString());
        wind_spd = d["observations"][0]["metric"]["windSpeed"] | 0.0f;
        wind_deg = d["observations"][0]["winddir"] | 0.0f;
        wind_gst = d["observations"][0]["metric"]["windGust"] | 0.0f;
        wind_ready = true;
        ATLAS_LOG(L_INFO, false, "[WIND] Synced: %.1f km/h\n", wind_spd);
    }
    http.end();
}

void checkModeOverride() {
    if(WiFi.status() != WL_CONNECTED) return;
    HTTPClient http; http.begin(MODE_OVERRIDE_URL);
    if(http.GET() == 200) {
        String m = http.getString(); m.trim();
        if(m=="admin" || m=="maintenance") currentMode = M_MAINTENANCE;
        else if(m=="deepsleep") currentMode = M_DEEP_SLEEP;
        else currentMode = M_CONTINUOUS;
    }
    http.end();
}

void pushData() {
    if(WiFi.status() != WL_CONNECTED) return;
    payload["timestamp"] = time(NULL);
    payload["mode"] = (currentMode==M_CONTINUOUS)?"Continuous":"Special";
    
    // Add wind to payload
    if(wind_ready) {
        payload["sensors"]["Wind_Speed"] = wind_spd;
        payload["sensors"]["Wind_Deg"] = wind_deg;
        payload["sensors"]["Wind_Gust"] = wind_gst;
    }

    // MQTT
    if(!mqtt.connected()) { mqtt.setServer(MQTT_SERVER, 1883); mqtt.connect(HOSTNAME, MQTT_USER, MQTT_PASS); }
    if(mqtt.connected()) { String s; serializeJson(payload, s); mqtt.publish(MQTT_TOPIC_STATE, s.c_str()); }

    // API
    WiFiClientSecure c; c.setInsecure(); HTTPClient h; h.begin(c, API_URL);
    h.addHeader("Content-Type", "application/json");
    String js; serializeJson(payload, js); h.POST(js); h.end();
}

// -----------------------------------------------------------------------
// 5. CORE SYSTEM
// -----------------------------------------------------------------------

void setup() {
    Serial.begin(115200); statusLed.begin(); statusLed.show();
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN); Wire.setTimeOut(200);
    
    ATLAS_LOG(L_BOOT_CRASH, true, "--- WeatherDNA ATLAS v2.0 ---\n");
    WiFi.begin(WIFI_SSIDS[0], WIFI_PASSWORD);
    
    // Hardware PCNT Geiger
    pcnt_config_t pcnt_config = {};
    pcnt_config.pulse_gpio_num = GEIGER_PIN; pcnt_config.unit = PCNT_UNIT_0;
    pcnt_config.pos_mode = PCNT_COUNT_INC; pcnt_unit_config(&pcnt_config);
    pcnt_counter_pause(PCNT_UNIT_0); pcnt_counter_clear(PCNT_UNIT_0); pcnt_counter_resume(PCNT_UNIT_0);
}

void loop() {
    esp_task_wdt_reset();
    if(currentMode == M_MAINTENANCE) handleDiagnosticMenu();

    switch(currentPhase) {
        case P_WAKEUP:
            payload.clear(); checkModeOverride();
            currentPhase = P_WARMUP; cycleStartTime = millis();
            break;
            
        case P_WARMUP:
            if(millis() - lastBsecPoll > 100) { if(muxSelect(MUX_WEATHER, CH0_BME688)) bme688.run(); lastBsecPoll=millis(); }
            if(millis() - cycleStartTime > dynamic_warmup_ms) currentPhase = P_READ;
            break;
            
        case P_READ:
            fetchWind();
            // Stevenson 0x72
            if(muxSelect(MUX_WEATHER, CH5_SHT45) && sht45.begin()) { 
                sensors_event_t h, t; sht45.getEvent(&h, &t);
                payload["sensors"]["Temp"] = t.temperature; payload["sensors"]["Hum"] = h.relative_humidity;
            }
            if(muxSelect(MUX_WEATHER, CH7_BMP585) && bmp585.begin()) {
                if(bmp585.performReading()) {
                    float p = bmp585.pressure/100.0f;
                    payload["sensors"]["Press_Raw"] = p;
                    payload["sensors"]["Press_SLP"] = calcSLP(p, payload["sensors"]["Temp"]|20.0f);
                }
            }
            // ZMOD4510... SCD41... etc logic here
            currentPhase = P_PUSH;
            break;
            
        case P_PUSH:
            pushData(); currentPhase = P_WAIT;
            break;
            
        case P_WAIT:
            if(currentMode == M_DEEP_SLEEP) {
                ATLAS_LOG(L_INFO, true, "Entering Deep Sleep...\n");
                esp_sleep_enable_timer_wakeup(300 * 1000000ULL); esp_deep_sleep_start();
            }
            delay(10000); currentPhase = P_WAKEUP;
            break;
    }
}
