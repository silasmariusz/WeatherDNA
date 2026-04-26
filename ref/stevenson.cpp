/*
 * STEVENSON SCREEN SUBSYSTEM (ATLAS Node)
 * Based on klatka_stevensona.md blueprint.
 * Handles purely the 0x72 I2C Multiplexer array for environmental sensing.
 */

#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <math.h>

// Sensor Libraries
#include <bsec2.h>
#include "no2_o3-arduino.h"          // Renesas ZMOD4510 SDK
#include <SensirionI2cScd4x.h>       // SCD41
#include <SensirionI2CSgp41.h>       // SGP41
#include <VOCGasIndexAlgorithm.h>
#include <NOxGasIndexAlgorithm.h>
#include "SparkFun_BMV080_Arduino_Library.h" // BMV080
#include "Adafruit_SHT4x.h"          // SHT45
#include <Adafruit_BMP5xx.h>         // BMP585

// --- NETWORK ---
const char* ssid = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";
const char* mqtt_server = "10.100.200.6";
WiFiClient espClient;
PubSubClient mqtt(espClient);

// --- I2C MULTIPLEXER (TCA9548A) ---
#define MUX_ADDR 0x72
const uint8_t CH_BME688   = 0;
const uint8_t CH_ZMOD4510 = 1;
const uint8_t CH_SCD41_SGP41 = 2;
const uint8_t CH_BMV080   = 3;
const uint8_t CH_SHT45    = 5;
const uint8_t CH_BMP585   = 7;

// --- SENSORS & ALGORITHMS ---
Bsec2 bme688;
zmod4xxx_dev_t zmod4510_dev;
no2_o3_handle_t zmod4510_algo_handle;
no2_o3_results_t zmod_results;
no2_o3_inputs_t zmod_inputs;
SensirionI2cScd4x scd41;
SensirionI2CSgp41 sgp41;
VOCGasIndexAlgorithm vocAlgo;
NOxGasIndexAlgorithm noxAlgo;
SparkFunBMV080 bmv080;
Adafruit_SHT4x sht45;
Adafruit_BMP5xx bmp585;

// --- STATE MACHINE ---
enum CyclePhase { PHASE_WAKEUP, PHASE_WARMUP, PHASE_READ, PHASE_PUSH_API, PHASE_SLEEP_WAIT };
CyclePhase currentPhase = PHASE_WAKEUP;
unsigned long cycleStartTime = 0;
unsigned long lastFastPollTime = 0;
int bmv_fail_count = 0;

// Data Buffers
float latest_temp = 0.0, latest_hum = 0.0, latest_press = 0.0;
float latest_pm25 = 0.0, latest_pm10 = 0.0;
int32_t voc_index = 0, nox_index = 0;
float pressure_history_3h[18] = {0}; // Simplified rolling buffer

void tcaselect(uint8_t i) {
    if (i > 7) return;
    Wire.beginTransmission(MUX_ADDR);
    Wire.write(1 << i);
    if (Wire.endTransmission() != 0) {
        Serial.println("[I2C WATCHDOG] Multiplexer failed to respond!");
        // Implementation of I2C Bus Reset
        Wire.end();
        delay(100);
        Wire.begin();
    }
}

// --- METEOROLOGICAL MATH (Step 4) ---
float calcDewPoint(float t, float h) {
    return t - ((100.0 - h) / 5.0);
}

float calcSeaLevelPressure(float p_abs, float t, float alt) {
    return p_abs * pow((1.0 - (0.0065 * alt) / (t + 0.0065 * alt + 273.15)), -5.257);
}

float calcAbsoluteHumidity(float t, float h) {
    return (6.112 * exp((17.67 * t) / (t + 243.5)) * h * 2.1674) / (273.15 + t);
}

float calcHeatIndex(float t, float h) {
    if (t <= 27.0 || h <= 40.0) return t;
    return t + 0.33 * (h / 100.0 * 6.105 * exp(17.27 * t / (237.7 + t))) - 4.0;
}

float calcSmogIndex(float pm25, float nox, float hum) {
    float pm_factor = pm25 / 10.0;
    float hum_multiplier = 1.0 + (max(0.0f, hum - 70.0f) / 100.0);
    float nox_factor = nox / 50.0;
    return min(10.0f, (pm_factor * hum_multiplier) + nox_factor);
}

float calcWHO_AQI(float pm25, float pm10) {
    float pm25_pct = (pm25 / 15.0) * 100.0;
    float pm10_pct = (pm10 / 45.0) * 100.0;
    return max(pm25_pct, pm10_pct);
}

// --- HARDWARE INIT ---
void setup() {
    Serial.begin(115200);
    Wire.begin();
    Wire.setTimeOut(200); // 200ms timeout for Bosch clock stretching

    WiFi.begin(ssid, password);
    mqtt.setServer(mqtt_server, 1883);

    Serial.println("[STEVENSON] Waking up 0x72 hardware...");
    
    tcaselect(CH_SHT45); sht45.begin(); sht45.setPrecision(SHT4X_HIGH_PRECISION);
    tcaselect(CH_BMP585); bmp585.begin();
    tcaselect(CH_SCD41_SGP41); scd41.begin(Wire, 0x62); scd41.stopPeriodicMeasurement(); sgp41.begin(Wire);
    tcaselect(CH_BMV080); bmv080.begin(0x57, Wire); bmv080.setMode(1); // Continuous
    tcaselect(CH_BME688); bme688.begin(0x77, Wire);
    
    cycleStartTime = millis();
    currentPhase = PHASE_WARMUP;
}

// --- MAIN STATE MACHINE ---
void loop() {
    unsigned long currentMillis = millis();

    switch(currentPhase) {
        case PHASE_WAKEUP:
            cycleStartTime = currentMillis;
            currentPhase = PHASE_WARMUP;
            break;
            
        case PHASE_WARMUP:
            // Wait ~12s for ZMOD and BMV080 to stabilize
            if (currentMillis - cycleStartTime >= 12000) {
                currentPhase = PHASE_READ;
                lastFastPollTime = currentMillis;
            }
            break;
            
        case PHASE_READ:
            // 1Hz High-Frequency Polling (Step 3)
            if (currentMillis - lastFastPollTime >= 1000) {
                lastFastPollTime = currentMillis;

                // BME688 AI algorithm pump
                tcaselect(CH_BME688);
                bme688.run(); 

                // SGP41 Raw Ticks -> Algorithm
                tcaselect(CH_SCD41_SGP41);
                uint16_t sraw_voc, sraw_nox;
                // Using default comp if SHT45 hasn't fired yet
                if (sgp41.measureRawSignals(0x8000, 0x6666, sraw_voc, sraw_nox) == 0) {
                    voc_index = vocAlgo.process(sraw_voc);
                    nox_index = noxAlgo.process(sraw_nox);
                }

                // BMV080 Dust Purge
                tcaselect(CH_BMV080);
                bmv080_output_t bmv_out;
                if (bmv080.readSensor(&bmv_out)) {
                    latest_pm25 = bmv_out.pm2_5_mass_concentration;
                    latest_pm10 = bmv_out.pm10_mass_concentration;
                    bmv_fail_count = 0;
                } else {
                    bmv_fail_count++;
                    if (bmv_fail_count > 15) {
                        Serial.println("[GLITCH GUARD] BMV080 Timeout - marking offline.");
                    }
                }
            }

            // Trigger synchronous read at 60s mark
            if (currentMillis - cycleStartTime >= 60000) {
                currentPhase = PHASE_PUSH_API;
            }
            break;
            
        case PHASE_PUSH_API:
            executeSynchronousReadsAndPush();
            currentPhase = PHASE_SLEEP_WAIT;
            break;
            
        case PHASE_SLEEP_WAIT:
            // Deep Sleep execution logic goes here (e.g. setMode(0) on BMV080)
            tcaselect(CH_BMV080); bmv080.setMode(0); // Laser OFF
            tcaselect(CH_SCD41_SGP41); scd41.powerDown();
            
            Serial.println("[STEVENSON] Cycle complete. Waiting for next WAKEUP interval.");
            delay(1000); // Simulating ESP light sleep or external RTC timer
            currentPhase = PHASE_WAKEUP;
            break;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        mqtt.loop();
    }
}

// --- SYNCHRONOUS ROUTINE (Step 3 & 5) ---
void executeSynchronousReadsAndPush() {
    JsonDocument payload;
    
    // 1. SHT45 (Read first for compensation)
    tcaselect(CH_SHT45);
    sensors_event_t hum, temp;
    if (sht45.getEvent(&hum, &temp)) {
        // Glitch Guard
        if (temp.temperature > -50.0 && temp.temperature < 85.0) {
            latest_temp = temp.temperature;
            latest_hum = hum.relative_humidity;
            payload["SHT45_Temp"] = latest_temp;
            payload["SHT45_Hum"] = latest_hum;
        }
    }

    // 2. BMP585
    tcaselect(CH_BMP585);
    if (bmp585.performReading()) {
        latest_press = bmp585.pressure / 100.0F; // hPa
        payload["BMP585_Pressure_hPa"] = latest_press;
    }

    // 3. SCD41 (Single Shot to avoid heating)
    tcaselect(CH_SCD41_SGP41);
    scd41.measureSingleShot();
    delay(5000); // SCD41 requires ~5s to complete single shot
    uint16_t co2; float st, sh;
    if (scd41.readMeasurement(co2, st, sh) == 0 && co2 > 0) {
        payload["SCD41_CO2_ppm"] = co2;
    }
    
    // Append fast-polled data
    payload["BMV080_PM2_5"] = latest_pm25;
    payload["SGP41_VOC_Index"] = voc_index;
    payload["SGP41_NOx_Index"] = nox_index;

    // 4. ZMOD4510 (Uses SHT45 data)
    tcaselect(CH_ZMOD4510);
    zmod_inputs.temperature_degc = latest_temp;
    zmod_inputs.humidity_pct = latest_hum;
    if (zmod4xxx_start_measurement(&zmod4510_dev) == 0) {
        zmod4510_dev.delay_ms(ZMOD4510_NO2_O3_SAMPLE_TIME);
        uint8_t z_adc[32];
        if (zmod4xxx_read_adc_result(&zmod4510_dev, z_adc) == 0) {
            zmod_inputs.adc_result = z_adc;
            if (calc_no2_o3(&zmod4510_algo_handle, &zmod4510_dev, &zmod_inputs, &zmod_results) == 0) {
                payload["ZMOD4510_NO2_ppb"] = zmod_results.NO2_conc_ppb;
                payload["ZMOD4510_O3_ppb"] = zmod_results.O3_conc_ppb;
            }
        }
    }

    // --- FUSION MATH (Step 4) ---
    if (latest_temp != 0.0 && latest_hum != 0.0) {
        payload["METEO_Dew_Point_C"] = calcDewPoint(latest_temp, latest_hum);
        payload["METEO_Abs_Hum_g_m3"] = calcAbsoluteHumidity(latest_temp, latest_hum);
        payload["METEO_Heat_Index"] = calcHeatIndex(latest_temp, latest_hum);
    }
    
    if (latest_press != 0.0) {
        payload["METEO_Sea_Level_Press_hPa"] = calcSeaLevelPressure(latest_press, latest_temp, 290.0);
    }

    payload["AIR_Smog_Index"] = calcSmogIndex(latest_pm25, nox_index, latest_hum);
    payload["AIR_WHO_AQI_Pct"] = calcWHO_AQI(latest_pm25, latest_pm10);

    // --- CLOUD PUSH (Step 5) ---
    if (mqtt.connected()) {
        String out;
        serializeJson(payload, out);
        mqtt.publish("airsense/state", out.c_str());
        Serial.println("[TELEMETRY] Successfully pushed to MQTT.");
    } else {
        Serial.println("[TELEMETRY] MQTT offline. Payload dropped.");
    }
}
