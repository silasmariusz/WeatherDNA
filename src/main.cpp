/*
********************************************************************************
*                                                                              *
*           WeatherDNA - ATLAS Environmental OS v2.2                           *
*           Full Implementation: All Hubs Integrated | Stevenson Optimized      *
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
#include <I2C_Addr_LS.h>
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

// ZMOD4510 Algos
static zmod4xxx_dev_t zmod_dev;
static uint8_t zmod_adc[ZMOD4510_ADC_DATA_LEN], zmod_prod[ZMOD4510_PROD_DATA_LEN];
static no2_o3_handle_t zmod_algo;
static no2_o3_results_t zmod_res;
static no2_o3_inputs_t zmod_in;
static Interface_t zmod_hal;

// Flags & Buffers
bool bme_ok=0, zmod_ok=0, bmv_ok=0, scd_ok=0, sgp_ok=0, sht_ok=0, bmp_ok=0, ilps_ok=0, bmm_ok=0, as3935_ok=0, veml_ok=0, i2c_mem_ok=0;
volatile uint32_t geiger_pulses = 0;
uint16_t sraw_voc = 0, sraw_nox = 0;

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
// 2. MUX WRAPPER & CORE MATH
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
// 3. SENSOR INITIALIZATION & DISCOVERY
// -----------------------------------------------------------------------

void bme688Callback(const bme68xData data, const bsecOutputs outputs, Bsec2 bsec) {
    if (!outputs.nOutputs) return;
    for (uint8_t i=0; i<outputs.nOutputs; i++) {
        switch (outputs.output[i].sensor_id) {
            case BSEC_OUTPUT_IAQ: payload["sensors"]["IAQ"] = outputs.output[i].signal; break;
            case BSEC_OUTPUT_CO2_EQUIVALENT: payload["sensors"]["eCO2"] = outputs.output[i].signal; break;
            case BSEC_OUTPUT_BREATH_VOC_EQUIVALENT: payload["sensors"]["bVOC"] = outputs.output[i].signal; break;
        }
    }
}

void initAllSensors() {
    ATLAS_LOG(L_INFO, true, "[INIT] Discovering Integrated Hubs...\n");
    
    // Stevenson (0x72)
    if (muxSelect(MUX_WEATHER, CH0_BME688) && bme688.begin(BME68X_I2C_ADDR_HIGH, Wire)) {
        bme688.setConfig(bsec_config_iaq); bme688.updateSubscription((bsecSensor[]){BSEC_OUTPUT_IAQ, BSEC_OUTPUT_CO2_EQUIVALENT, BSEC_OUTPUT_BREATH_VOC_EQUIVALENT}, 3, BSEC_SAMPLE_RATE_LP);
        bme688.attachCallback(bme688Callback); bme_ok = true; ATLAS_LOG(L_INFO, true, " |- CH0: BME688 AI -> ONLINE\n");
    }
    if (muxSelect(MUX_WEATHER, CH1_ZMOD4510)) {
        HAL_Init(&zmod_hal); zmod_dev.i2c_addr = ZMOD4510_I2C_ADDR; zmod_dev.pid = ZMOD4510_PID;
        zmod_dev.init_conf = &zmod_no2_o3_sensor_cfg[INIT]; zmod_dev.meas_conf = &zmod_no2_o3_sensor_cfg[MEASUREMENT]; zmod_dev.prod_data = zmod_prod;
        if (zmod4xxx_init(&zmod_dev, &zmod_hal) == 0) { init_no2_o3(&zmod_algo); zmod_ok = true; ATLAS_LOG(L_INFO, true, " |- CH1: ZMOD4510 -> ONLINE\n"); }
    }
    if (muxSelect(MUX_WEATHER, CH2_GAS_NDIR)) { scd41.begin(Wire, 0x62); sgp41.begin(Wire); scd_ok = sgp_ok = true; ATLAS_LOG(L_INFO, true, " |- CH2: SCD41+SGP41 -> ONLINE\n"); }
    if (muxSelect(MUX_WEATHER, CH3_BMV080) && bmv080.begin()) { bmv080.init(); bmv080.setMode(1); bmv_ok = true; ATLAS_LOG(L_INFO, true, " |- CH3: BMV080 Dust -> ONLINE\n"); }
    if (muxSelect(MUX_WEATHER, CH5_SHT45) && sht45.begin()) { sht_ok = true; ATLAS_LOG(L_INFO, true, " |- CH5: SHT45 Ref -> ONLINE\n"); }
    if (muxSelect(MUX_WEATHER, CH7_BMP585) && bmp585.begin()) { bmp_ok = true; ATLAS_LOG(L_INFO, true, " |- CH7: BMP585 Baro -> ONLINE\n"); }

    // Optics (0x71)
    if (muxSelect(MUX_OPTICS, CH3_VEML7700) && veml.begin()) { veml_ok = true; ATLAS_LOG(L_INFO, true, " |- CH3: VEML7700 Lux -> ONLINE\n"); }
    if (muxSelect(MUX_OPTICS, CH6_ILPS22QS) && ilps.begin() == 0) { ilps.Enable(); ilps_ok = true; ATLAS_LOG(L_INFO, true, " |- CH6: ILPS22QS QVAR -> ONLINE\n"); }
}

// -----------------------------------------------------------------------
// 4. DATA COLLECTION & Raporting
// -----------------------------------------------------------------------

void exhaustivelyRead() {
    ATLAS_LOG(L_INFO, true, "[READ] Sampling Stevenson Screen...\n");
    
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient h; h.begin("http://api.weather.com/v2/pws/observations/current?stationId=" + String(WU_STATION_ID) + "&format=json&units=m&apiKey=e1f10a1e78194ce3b10a1e7819ece351");
        if(h.GET() == 200) { JsonDocument d; deserializeJson(d, h.getString()); wind_spd = d["observations"][0]["metric"]["windSpeed"]; wind_deg = d["observations"][0]["winddir"]; payload["sensors"]["Wind_Spd"] = wind_spd; payload["sensors"]["Wind_Deg"] = wind_deg; wind_ready=1; }
        h.end();
    }

    if(sht_ok && muxSelect(MUX_WEATHER, CH5_SHT45)) { sensors_event_t h, t; sht45.getEvent(&h, &t); payload["sensors"]["Temp"] = t.temperature; payload["sensors"]["Hum"] = h.relative_humidity; }
    if(bmp_ok && muxSelect(MUX_WEATHER, CH7_BMP585) && bmp585.performReading()) { float p = bmp585.pressure/100.0f; payload["sensors"]["Press_Raw"] = p; payload["sensors"]["Press_SLP"] = calcSLP(p, payload["sensors"]["Temp"] | 20.0f); }
    if(scd_ok && muxSelect(MUX_WEATHER, CH2_GAS_NDIR)) { uint16_t c,h; float t; if(scd41.readMeasurement(c,t,h)==0) payload["sensors"]["SCD_CO2"] = c; }
    if(sgp_ok && muxSelect(MUX_WEATHER, CH2_GAS_NDIR)) { sgp41.measureRawSignals(0x8000, 0x6666, sraw_voc, sraw_nox); payload["sensors"]["VOC_Idx"] = vocAlgo.process(sraw_voc); payload["sensors"]["NOx_Idx"] = noxAlgo.process(sraw_nox); }
    if(bmv_ok && muxSelect(MUX_WEATHER, CH3_BMV080)) { bmv080_output_t out; if(bmv080.readSensor(&out)) payload["sensors"]["PM2_5"] = out.pm2_5_mass_concentration; }
    if(zmod_ok && muxSelect(MUX_WEATHER, CH1_ZMOD4510)) { if(zmod4xxx_read_adc(&zmod_dev, zmod_adc) == 0) { zmod_in.adc_result = zmod_adc; if(calc_no2_o3(&zmod_algo, &zmod_dev, &zmod_in, &zmod_res) == 0) { payload["sensors"]["O3_ppb"] = zmod_res.o3_conc_ppb; payload["sensors"]["NO2_ppb"] = zmod_res.no2_conc_ppb; } } }
    if(veml_ok && muxSelect(MUX_OPTICS, CH3_VEML7700)) { payload["sensors"]["Lux"] = veml.readLux(); }
    if(ilps_ok && muxSelect(MUX_OPTICS, CH6_ILPS22QS)) { ILPS22QS_Data_t d; ilps.Get_Data(&d); payload["sensors"]["QVAR"] = d.qvar; }

    Serial1.print("r\n"); delay(100); if(Serial1.available()) { String r = Serial1.readStringUntil('\n'); if(r.indexOf("Acc")>=0) payload["sensors"]["Rain_Acc"] = r.substring(r.indexOf("Acc")+4).toFloat(); }
}

void pushData() {
    if(WiFi.status() != WL_CONNECTED) return;
    payload["timestamp"] = time(NULL); payload["uptime"] = millis()/3600000.0;
    if(!mqtt.connected()) { mqtt.setServer(MQTT_SERVER, 1883); mqtt.connect(HOSTNAME, MQTT_USER, MQTT_PASS); }
    if(mqtt.connected()) { String s; serializeJson(payload, s); mqtt.publish(MQTT_TOPIC_STATE, s.c_str()); }
    WiFiClientSecure c; c.setInsecure(); HTTPClient h; h.begin(c, API_URL); h.addHeader("Content-Type", "application/json");
    String js; serializeJson(payload, js); h.POST(js); h.end();
}

// -----------------------------------------------------------------------
// 5. DIAGNOSTICS & OTA
// -----------------------------------------------------------------------

void i2cScanner() {
    ATLAS_LOG(L_INFO, true, "\n--- SMART I2C SCANNER (WeatherDNA Mapping) ---\n");
    static std::vector<uint8_t> last_found;
    while(!Serial.available()) {
        std::vector<uint8_t> current;
        for(int i=0; addr_list[i].name != NULL; i++) {
            Wire.beginTransmission(addr_list[i].addr);
            if(Wire.endTransmission() == 0) {
                current.push_back(addr_list[i].addr); bool is_new = true; for(uint8_t o : last_found) if(o==addr_list[i].addr) is_new=false;
                Serial.printf("[0x%02X: %s%s] ", addr_list[i].addr, addr_list[i].name, is_new?"*":"");
            }
        }
        Serial.println(); last_found = current; delay(2000);
    }
    Serial.read();
}

void setupOTA() {
    ArduinoOTA.setHostname(HOSTNAME);
    ArduinoOTA.onStart([]() { ATLAS_LOG(L_INFO, true, "OTA Start\n"); });
    ArduinoOTA.begin();
}

// -----------------------------------------------------------------------
// 6. MAIN LOOP
// -----------------------------------------------------------------------

void setup() {
    Serial.begin(115200); Serial1.begin(9600, SERIAL_8N1, RAIN_RG15_RX, RAIN_RG15_TX);
    statusLed.begin(); statusLed.show(); Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN); Wire.setTimeOut(200);
    ATLAS_LOG(L_BOOT_CRASH, true, "--- WeatherDNA v2.2 BOOT ---\n");
    WiFi.begin(WIFI_SSIDS[0], WIFI_PASSWORD);
    
    // Hardware PCNT
    pcnt_config_t pcnt_config = {}; pcnt_config.pulse_gpio_num = GEIGER_PIN; pcnt_config.unit = PCNT_UNIT_0; pcnt_config.pos_mode = PCNT_COUNT_INC; pcnt_unit_config(&pcnt_config);
    pcnt_counter_pause(PCNT_UNIT_0); pcnt_counter_clear(PCNT_UNIT_0); pcnt_counter_resume(PCNT_UNIT_0);
    
    initAllSensors(); setupOTA();
}

void loop() {
    esp_task_wdt_reset(); ArduinoOTA.handle();
    if(currentMode == M_MAINTENANCE) { if(Serial.available()) { char c = Serial.read(); if(c=='s') i2cScanner(); if(c=='m') currentMode=M_CONTINUOUS; } }

    switch(currentPhase) {
        case P_WAKEUP: payload.clear(); currentPhase = P_WARMUP; cycleStartTime = millis(); break;
        case P_WARMUP: if(millis() - lastBsecPoll > 100) { if(muxSelect(MUX_WEATHER, CH0_BME688)) bme688.run(); lastBsecPoll=millis(); }
                       if(millis() - cycleStartTime > dynamic_warmup_ms) currentPhase = P_READ; break;
        case P_READ: exhaustivelyRead(); currentPhase = P_PUSH; break;
        case P_PUSH: pushData(); currentPhase = P_WAIT; break;
        case P_WAIT: delay(15000); currentPhase = P_WAKEUP; break;
    }
}
