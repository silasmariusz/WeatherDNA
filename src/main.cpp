/*
 * A.T.L.A.S. Node v3 - Klatka Stevensona (Clean Refactor)
 * Board: ESP32-S3 DevKitC-1 (16MB FLASH / 8MB PSRAM)
 * Wyłącznie czujniki środowiskowe na szynie I2C z MUX 0x72
 */

#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <ArduinoOTA.h>
#include <esp_task_wdt.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <math.h>

// --- WWW DASHBOARD ---
#include "www_index.h"

// --- BSEC3 (BME690 AI) Bare Metal ---
#include "bme68x.h"
#include "bsec_interface.h"
extern "C" {
#include "../lib/bsec_iaq/bsec_iaq.h"
}
#include "esp_timer.h" 

// --- SENSORS ---
// Nicla Sense Env Placeholder
#include <SensirionI2cScd4x.h>       // SCD41
#include <SensirionI2CSgp41.h>       // SGP41
#include <VOCGasIndexAlgorithm.h>
#include <NOxGasIndexAlgorithm.h>
#include "SparkFun_BMV080_Arduino_Library.h" // BMV080
#include "Adafruit_SHT4x.h"          // SHT45
#include <Adafruit_BMP5xx.h>         // BMP585
#include <ILPS22QSSensor.h>          // ILPS22QS
#include <extEEPROM.h>               // I2C EEPROM (BSEC State)
#include <DFRobot_AS3935_I2C.h>      // AS3935 Lightning

// --- DIAGNOSTICS ---
#define ADMIN_MODE true
#include <I2C_Addr_LS.h>             // Lokalna biblioteka adresów I2C

// --- BIOMETEO CORE ---
#include "Biometeo.h"
#include "AirQualityIndices.h"
#include "bsec_datatypes.h"
#include "AirQualityIndices.h"
#include "bsec_datatypes.h"

#include <stdarg.h>
#define MAX_LOG_LINES 50
String logBuffer[MAX_LOG_LINES];
int logHead = 0;
int logCount = 0;

void ATLAS_LOG(const char* format, ...) {
    char buf[512];
    va_list args;
    va_start(args, format);
    vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);
    
    Serial.print(buf);
    
    logBuffer[logHead] = String(buf);
    logHead = (logHead + 1) % MAX_LOG_LINES;
    if (logCount < MAX_LOG_LINES) logCount++;
}

// --- CONFIG ---
#define I2C_SDA_PIN 4
#define I2C_SCL_PIN 5

#define RG15_TX 43           // Serial UART TX
#define RG15_RX 44           // Serial UART RX
const float STATION_ALTITUDE = 290.0f;

bool repairWiFi();

// ᗧ···ᗣ···ᗣ····ᗣ····ᗣ····ᗣ····ᗣ····ᗣ····ᗣ····ᗣ····ᗣ····ᗣ····ᗣ····ᗣ
// 1. NETWORK, API, GPS & MQTT CONFIGURATION
// ᗧ···ᗣ···ᗣ····ᗣ····ᗣ····ᗣ····ᗣ····ᗣ····ᗣ····ᗣ····ᗣ····ᗣ····ᗣ····ᗣ


// -------------- Passwords to your APIs --------------- //

#include "secrets.h"

static constexpr size_t WIFI_SSID_COUNT = sizeof(WIFI_SSIDS) / sizeof(WIFI_SSIDS[0]);

const char* HOSTNAME        = "AirSense-Node";


const int MQTT_PORT = 1883;
const char* MQTT_TOPIC_STATE = "airsense/state";
static const char* TZ_INFO = "CET-1CEST,M3.5.0,M10.5.0/3";

// --- KONFIGURACJA QNAP SYSLOG (QuLog Center) ---
const int SYSLOG_PORT = 1514;
static const bool ENABLE_SYSLOG = false; // DISABLED - set to true to enable syslog logging

//WiFiUDP syslogUdp;

static const char* NTP_SERVER_1 = "pool.ntp.org";
static const char* NTP_SERVER_2 = "tempus1.gum.gov.pl";

static constexpr uint32_t NTP_TIMEOUT_MS = 5000;

// WIFI AUTO-REPAIR FUNCTION
// Sprawdza połączenie WiFi i wykonuje procedurę naprawy jeśli potrzebna
// Zwraca true jeśli połączenie jest ok lub udało się naprawić, false jeśli całkowicie nieudane
bool repairWiFi() {
    const unsigned long CONNECT_TIMEOUT_MS = 15000; // 15 sekund na połączenie
    const int MAX_RETRIES = 3;

    // 1. Sprawdź czy WiFi jest połączone
    if (WiFi.status() == WL_CONNECTED) {
        // Test rzeczywistej łączności - próba połączenia z API
        WiFiClient testClient;
			// tu trzeba wyekstrakt
        if (testClient.connect(API_ICMP, 80)) {
            testClient.stop();
            return true; // WiFi działa poprawnie
        }
        ATLAS_LOG("[WIFI] Connected but no route to internet. Attempting repair...\n");
    } else {
        ATLAS_LOG("[WIFI] WiFi disconnected. Starting repair procedure...\n");
    }

    // 2. Procedura naprawy
    for (int attempt = 1; attempt <= MAX_RETRIES; attempt++) {
        ATLAS_LOG("[WIFI] Repair attempt %d/%d...\n", attempt, MAX_RETRIES);


		// DELAY? WTF
        // Rozłącz i wyczyść konfigurację
        WiFi.disconnect(true);
        delay(500);
        WiFi.mode(WIFI_STA);
        delay(200);

        // Rozpocznij próbę połączenia
        ATLAS_LOG("[WIFI] Connecting to SSID: %s\n", WIFI_SSIDS[0]);
        WiFi.begin(WIFI_SSIDS[0], WIFI_PASSWORD);

        // Czekaj z timeout
        unsigned long start = millis();
        while (millis() - start < CONNECT_TIMEOUT_MS) {
            if (WiFi.status() == WL_CONNECTED) {
                // Test łączności
                WiFiClient testClient;
                if (testClient.connect("zyndrama52.devspark.pl", 80)) {
                    testClient.stop();
                    ATLAS_LOG("[WIFI] Successfully repaired! Connected in %lu ms.\n", millis() - start);
                    return true;
                }
            }
            delay(100);
            yield();
        }

        ATLAS_LOG("[WIFI] Attempt %d failed. %s\n", attempt, (attempt < MAX_RETRIES) ? "Retrying..." : "Giving up.");
    }

    // 3. Całkowity błąd - kontynuuj ale z ostrzeżeniem
    ATLAS_LOG("[WIFI] WARNING: Could not repair WiFi connection. Proceeding with limited functionality.\n");
    return false;
}


// --- MUX 0x72 TOPOLOGY ---
const uint8_t MUX_ADDR = 0x72;
const uint8_t CH_BME688   = 0;
const uint8_t CH_ZMOD4510 = 1;
const uint8_t CH_SCD41_SGP41 = 2;
const uint8_t CH_BMV080   = 3;
const uint8_t CH_AS3935   = 4; // Detektor Burz
const uint8_t CH_SHT45    = 5;
const uint8_t CH_ILPS22QS = 6;
const uint8_t CH_BMP585   = 7;

// --- GLOBALS ---
WiFiClient espClient;
PubSubClient mqtt(espClient);
WebServer server(80);
JsonDocument payload;

enum SystemMode { MODE_CONTINUOUS, MODE_MAINTENANCE, MODE_RECOVERY };
SystemMode currentMode = MODE_CONTINUOUS;

enum CyclePhase { PHASE_WAKEUP, PHASE_WARMUP, PHASE_READ, PHASE_PUSH_API, PHASE_SLEEP_WAIT };
CyclePhase currentPhase = PHASE_WAKEUP;
unsigned long cycleStartTime = 0;
unsigned long lastFastPollTime = 0;

// Zmienne dla Math/Fuzji danych
float latest_temp = 0.0, latest_hum = 0.0, latest_press = 0.0;
float latest_pm25 = 0.0, latest_pm10 = 0.0;
int32_t voc_index = 0, nox_index = 0;

static int8_t bme68x_i2c_read(uint8_t, uint8_t*, uint32_t, void*);
static int8_t bme68x_i2c_write(uint8_t, const uint8_t*, uint32_t, void*);
static void bme68x_delay_us(uint32_t, void*);

// --- SENSOR OBJECTS ---
struct bme68x_dev bme;
uint8_t bsec_instance[3272];
uint8_t bsec_work_buffer[BSEC_MAX_WORKBUFFER_SIZE];
bool bme690_bsec_ready = false;
uint8_t bme_last_op_mode = BME68X_SLEEP_MODE;
bsec_bme_settings_t bsec_sensor_settings;

extEEPROM eeprom(kbits_256, 1, 64, 0x50); bool eeprom_ok = false;

// Nicla Sense Env Placeholder
bool nicla_ok = false;
SensirionI2cScd4x scd41;    bool scd41_ok = false; bool scd41_triggered = false;
SensirionI2CSgp41 sgp41;    bool sgp41_ok = false;
VOCGasIndexAlgorithm vocAlgo;
NOxGasIndexAlgorithm noxAlgo;
SparkFunBMV080 bmv080;      bool bmv080_ok = false;
Adafruit_SHT4x sht45;       bool sht45_ok = false;
ILPS22QSSensor ilps(&Wire); bool ilps_ok = false;
Adafruit_BMP5xx bmp585;     bool bmp585_ok = false;
DFRobot_AS3935_I2C as3935(0x03, 0); bool as3935_ok = false;

int i2c_fail_count = 0;



HardwareSerial RainSerial(1);
String rg15_buffer = "";
float daily_rain = 0.0;

// --- I2C MUX ---
bool tcaselect(uint8_t i) {
    if (i > 7) return false;
    Wire.beginTransmission(MUX_ADDR);
    Wire.write(1 << i);
    if (Wire.endTransmission() != 0) {
        i2c_fail_count++;
        if (i2c_fail_count > 3) {
            ATLAS_LOG("[I2C WATCHDOG] Bus Latch-Up detected! Resetting Wire...\n");
            Wire.end(); 
            delay(100); 
            Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN); 
            Wire.setClock(100000);
            i2c_fail_count = 0;
        }
        return false;
    }
    i2c_fail_count = 0;
    delay(5); // Czas na ustabilizowanie pojemności kabla Klatki Stevensona
    return true;
}

// --- ADMIN I2C DIAGNOSTIC SCANNER ---
String identifyI2CDevice(uint8_t addr) {
    // 1. Priorytet: Sensory Klatki Stevensona (zdefiniowane w SKILL.md)
    switch(addr) {
        case 0x72: return "( Stevenson MUX ) TCA9548A";
        case 0x77: return "( Stevenson ) BME688 AI Gas";
        case 0x03: return "( Stevenson ) AS3935 Lightning";
        case 0x33: return "( Stevenson ) ZMOD4510 NO2/O3";
        case 0x62: return "( Stevenson ) SCD41 CO2";
        case 0x59: return "( Stevenson ) SGP41 VOC/NOx";
        case 0x57: return "( Stevenson ) BMV080 Dust";
        case 0x44: return "( Stevenson ) SHT45 Temp/Hum";
        case 0x5C: return "( Stevenson ) ILPS22QS Press";
        case 0x46: return "( Stevenson ) BMP585 Press";
        case 0x50: return "( Main Bus ) EEPROM BSEC State";
    }
    
    // 2. Fallback: Baza danych I2C_Addr_LS (lokalnie w d:\Arduino\code\I2C_Addr_LS\)
    // Odkomentuj i dostosuj poniższą linię, jeśli API biblioteki zwraca klasę/nazwę statycznie:
    // return I2C_Addr_LS::getName(addr); 
    
    return "Unknown / External";
}

void runAdminI2CScan() {
    ATLAS_LOG("\n==================================================\n");
    ATLAS_LOG("   [ADMIN MODE] HARDWARE I2C DIAGNOSTIC SCAN\n");
    ATLAS_LOG("==================================================\n");
    
    // Twarde zamkniecie MUX przed skanem bazy
    Wire.beginTransmission(MUX_ADDR); Wire.write(0); Wire.endTransmission();
    ATLAS_LOG("\n--- MAIN BUS SCAN (NO MUX) ---\n");
    int main_devices = 0;
    for (uint8_t a = 1; a < 127; a++) {
        Wire.beginTransmission(a);
        if (Wire.endTransmission() == 0) {
            ATLAS_LOG("FOUND: 0x%02X -> %s\n", a, identifyI2CDevice(a).c_str());
            main_devices++;
        }
    }
    if (main_devices == 0) ATLAS_LOG("No devices found on main bus.\n");

    ATLAS_LOG("\n--- MUX CHANNELS SCAN ---\n");
    Wire.beginTransmission(MUX_ADDR);
    if (Wire.endTransmission() != 0) {
        ATLAS_LOG("CRITICAL FAULT: MUX at 0x%02X not responding! Check wiring.\n", MUX_ADDR);
    } else {
        for (uint8_t ch = 0; ch < 8; ch++) {
            ATLAS_LOG("\nScanning MUX CH%d...\n", ch);
            if (!tcaselect(ch)) {
                ATLAS_LOG("  [!] Failed to open CH%d - Latch-Up or MUX damaged!\n", ch);
                continue;
            }
            int ch_devices = 0;
            for (uint8_t a = 1; a < 127; a++) {
                if (a == MUX_ADDR) continue; // Pomiń sam MUX
                Wire.beginTransmission(a);
                if (Wire.endTransmission() == 0) {
                    ATLAS_LOG("  FOUND: 0x%02X -> %s\n", a, identifyI2CDevice(a).c_str());
                    ch_devices++;
                }
            }
            if (ch_devices == 0) ATLAS_LOG("  (Empty channel)\n");
        }
        // Zamknięcie wszystkich kanałów po skanowaniu, aby nie zakłócać Main Bus
        Wire.beginTransmission(MUX_ADDR);
        Wire.write(0); Wire.endTransmission(); Wire.endTransmission();
        Wire.endTransmission();
    }
    ATLAS_LOG("==================================================\n\n");
}

// --- METEOROLOGICAL MATH & FUSION ---
BiometeoCore biometeo;



// --- BME68x I2C Wrappers (BSEC3 Bare Metal) ---
static int8_t bme68x_i2c_read(uint8_t reg_addr, uint8_t *reg_data, uint32_t length, void *intf_ptr) {
    tcaselect(CH_BME688); // Tarcza Anti-Latch-Up
    Wire.beginTransmission(0x77);
    Wire.write(reg_addr);
    if (Wire.endTransmission(false) != 0) return -1;
    uint8_t bytes = Wire.requestFrom((uint8_t)0x77, (uint8_t)length);
    for(int i=0; i<bytes; i++) reg_data[i] = Wire.read();
    return (bytes == length) ? 0 : -1;
}

static int8_t bme68x_i2c_write(uint8_t reg_addr, const uint8_t *reg_data, uint32_t length, void *intf_ptr) {
    tcaselect(CH_BME688);
    Wire.beginTransmission(0x77);
    Wire.write(reg_addr);
    for(int i=0; i<length; i++) Wire.write(reg_data[i]);
    return (Wire.endTransmission() == 0) ? 0 : -1;
}

static void bme68x_delay_us(uint32_t period, void *intf_ptr) {
    uint32_t ms = (period + 999) / 1000;
    vTaskDelay(pdMS_TO_TICKS(ms ? ms : 1));
}

// --- HARDWARE INIT ---
void initSensors() {
    ATLAS_LOG("\n[SYSTEM] Waking up Stevenson Screen 0x72 hardware...\n");
    
    // Zabezpieczenie przed brakiem fizycznego podłączenia Klatki Stevensona
    Wire.beginTransmission(MUX_ADDR);
    if (Wire.endTransmission() != 0) {
        ATLAS_LOG("[ERROR] MUX 0x72 NOT FOUND! Halting environmental init.\n");
        return;
    }

    // EEPROM (Poza MUX, szyna główna)
    if (eeprom.begin(extEEPROM::twiClock100kHz, &Wire) == 0) {
        eeprom_ok = true;
        ATLAS_LOG("  |- EEPROM 0x50 (BSEC State): ONLINE\n");
    }

    if (tcaselect(CH_BME688)) {
        bme.intf = BME68X_I2C_INTF;
        bme.read = bme68x_i2c_read;
        bme.write = bme68x_i2c_write;
        bme.delay_us = bme68x_delay_us;
        bme.intf_ptr = NULL;
        bme.amb_temp = 25;

        if (bme68x_init(&bme) == BME68X_OK) {
            if (bsec_init() == BSEC_OK) {
                bsec_set_configuration(bsec_config_iaq, sizeof(bsec_config_iaq), bsec_work_buffer, sizeof(bsec_work_buffer));

                if (eeprom_ok) {
                    uint8_t state[BSEC_MAX_STATE_BLOB_SIZE] = {0};
                    if (eeprom.read(0, state, BSEC_MAX_STATE_BLOB_SIZE) == 0) {
                        bsec_set_state(state, BSEC_MAX_STATE_BLOB_SIZE, bsec_work_buffer, sizeof(bsec_work_buffer));
                    }
                }
                
                bsec_sensor_configuration_t requested[] = {
                    { BSEC_SAMPLE_RATE_LP, BSEC_OUTPUT_IAQ },
                    { BSEC_SAMPLE_RATE_LP, BSEC_OUTPUT_STATIC_IAQ },
                    { BSEC_SAMPLE_RATE_LP, BSEC_OUTPUT_CO2_EQUIVALENT },
                    { BSEC_SAMPLE_RATE_LP, BSEC_OUTPUT_BREATH_VOC_EQUIVALENT },
                    { BSEC_SAMPLE_RATE_LP, BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE },
                    { BSEC_SAMPLE_RATE_LP, BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY },
                    { BSEC_SAMPLE_RATE_LP, BSEC_OUTPUT_RAW_PRESSURE }
                };
                bsec_sensor_configuration_t required_settings[BSEC_MAX_PHYSICAL_SENSOR];
                uint8_t n_required = BSEC_MAX_PHYSICAL_SENSOR;

                if (bsec_update_subscription(requested, 7, required_settings, &n_required) == BSEC_OK) {
                    bme690_bsec_ready = true;
                    ATLAS_LOG("  |- BME690 (BSEC3 Bare Metal): ONLINE\n");
                }
            }
        }
    }

    if (tcaselect(CH_AS3935)) {
        if (as3935.begin() == 0) {
            as3935_ok = true;
            ATLAS_LOG("  |- AS3935 (Lightning Detector): ONLINE\n");
        }
    }

    // Nicla Sense Env Initialization placeholder (na kanale MUX, np. 1)
    if (tcaselect(CH_ZMOD4510)) {
        // Docelowo: if (BHY2.begin(Wire)) { nicla_ok = true; Serial.println("  |- Nicla Sense Env: ONLINE"); }
        ATLAS_LOG("  |- Nicla Sense Env Placeholder (3.3V Logic): READY\n");
    }

    if (tcaselect(CH_SCD41_SGP41)) {
        scd41.begin(Wire, 0x62); 
        if (scd41.stopPeriodicMeasurement() == 0) {
            scd41_ok = true;
            ATLAS_LOG("  |- SCD41 (CO2): ONLINE\n");
        }
        sgp41.begin(Wire); 
        sgp41_ok = true;
        ATLAS_LOG("  |- SGP41 (VOC/NOx): ONLINE\n");
    }

    if (tcaselect(CH_BMV080)) {
        if (bmv080.begin(0x57, Wire)) { 
            bmv080.init(); 
            bmv080.setMode(1); 
            bmv080_ok = true; 
            ATLAS_LOG("  |- BMV080 (Fanless Dust): ONLINE\n"); 
        }
    }

    if (tcaselect(CH_SHT45)) {
        if (sht45.begin()) { 
            sht45.setPrecision(SHT4X_HIGH_PRECISION); 
            sht45_ok = true; 
            ATLAS_LOG("  |- SHT45 (Prec. Temp/Hum): ONLINE\n"); 
        }
    }

    if (tcaselect(CH_ILPS22QS)) {
        if (ilps.begin() == 0 && ilps.Enable() == 0) { 
            ilps_ok = true; 
            ATLAS_LOG("  |- ILPS22QS (QVAR/Press): ONLINE\n"); 
        }
    }

    if (tcaselect(CH_BMP585)) {
        if (bmp585.begin()) { 
            bmp585_ok = true; 
            ATLAS_LOG("  |- BMP585 (Zambretti Core): ONLINE\n"); 
        }
    }

    // Peryferia główne
    RainSerial.begin(9600, SERIAL_8N1, RG15_RX, RG15_TX);
    ATLAS_LOG("  |- RG-15 (Rain UART): ONLINE\n");

    
}

// --- MAIN LOOP EXECUTION ---
void readFastSensors() {
    unsigned long currentMillis = millis();
    if (currentMillis - lastFastPollTime >= 1000) {
        lastFastPollTime = currentMillis;

        if (bme690_bsec_ready) {
            tcaselect(CH_BME688);
            int64_t timestamp_ns = esp_timer_get_time() * 1000LL;
            if (bsec_sensor_control(timestamp_ns, &bsec_sensor_settings) == BSEC_OK) {
                if (bsec_sensor_settings.trigger_measurement) {
                    struct bme68x_conf conf;
                    conf.os_hum = bsec_sensor_settings.humidity_oversampling;
                    conf.os_temp = bsec_sensor_settings.temperature_oversampling;
                    conf.os_pres = bsec_sensor_settings.pressure_oversampling;
                    conf.filter = BME68X_FILTER_OFF;
                    conf.odr = BME68X_ODR_NONE;
                    bme68x_set_conf(&conf, &bme);

                    if (bsec_sensor_settings.op_mode == BME68X_FORCED_MODE) {
                        bme68x_set_op_mode(BME68X_SLEEP_MODE, &bme);
                        struct bme68x_heatr_conf heatr_conf;
                        heatr_conf.enable = BME68X_ENABLE;
                        heatr_conf.heatr_temp = bsec_sensor_settings.heater_temperature;
                        heatr_conf.heatr_dur = bsec_sensor_settings.heater_duration;
                        bme68x_set_heatr_conf(BME68X_FORCED_MODE, &heatr_conf, &bme);
                        bme68x_set_op_mode(BME68X_FORCED_MODE, &bme);
                        bme_last_op_mode = BME68X_FORCED_MODE;

                        uint32_t meas_dur = bme68x_get_meas_dur(BME68X_FORCED_MODE, &conf, &bme);
                        bme68x_delay_us(meas_dur + (bsec_sensor_settings.heater_duration * 1000), NULL);

                        struct bme68x_data sensor_data[3];
                        uint8_t n_fields;
                        if (bme68x_get_data(BME68X_FORCED_MODE, sensor_data, &n_fields, &bme) == BME68X_OK && n_fields > 0) {
                            bsec_input_t inputs[BSEC_MAX_PHYSICAL_SENSOR];
                            uint8_t n_inputs = 0;

                            if (bsec_sensor_settings.process_data & BSEC_PROCESS_TEMPERATURE) {
                                inputs[n_inputs].sensor_id = BSEC_INPUT_TEMPERATURE;
                                inputs[n_inputs].signal = sensor_data[0].temperature;
                                inputs[n_inputs].time_stamp = timestamp_ns;
                                inputs[n_inputs].signal_dimensions = 1;
                                n_inputs++;
                            }
                            if (bsec_sensor_settings.process_data & BSEC_PROCESS_HUMIDITY) {
                                inputs[n_inputs].sensor_id = BSEC_INPUT_HUMIDITY;
                                inputs[n_inputs].signal = sensor_data[0].humidity;
                                inputs[n_inputs].time_stamp = timestamp_ns;
                                inputs[n_inputs].signal_dimensions = 1;
                                n_inputs++;
                            }
                            if (bsec_sensor_settings.process_data & BSEC_PROCESS_PRESSURE) {
                                inputs[n_inputs].sensor_id = BSEC_INPUT_PRESSURE;
                                inputs[n_inputs].signal = sensor_data[0].pressure;
                                inputs[n_inputs].time_stamp = timestamp_ns;
                                inputs[n_inputs].signal_dimensions = 1;
                                n_inputs++;
                            }
                            if (bsec_sensor_settings.process_data & BSEC_PROCESS_GAS) {
                                if (sensor_data[0].status & BME68X_GASM_VALID_MSK) {
                                    inputs[n_inputs].sensor_id = BSEC_INPUT_GASRESISTOR;
                                    inputs[n_inputs].signal = sensor_data[0].gas_resistance;
                                    inputs[n_inputs].time_stamp = timestamp_ns;
                                    inputs[n_inputs].signal_dimensions = 1;
                                    n_inputs++;
                                }
                            }

                            bsec_output_t bsec_outputs[BSEC_NUMBER_OUTPUTS];
                            uint8_t n_bsec_outputs = BSEC_NUMBER_OUTPUTS;
                            if (bsec_do_steps(inputs, n_inputs, bsec_outputs, &n_bsec_outputs) == BSEC_OK) {
                                for (uint8_t i = 0; i < n_bsec_outputs; i++) {
                                    if (bsec_outputs[i].sensor_id == BSEC_OUTPUT_IAQ) {
                                        payload["BME688_IAQ"] = bsec_outputs[i].signal;
                                        payload["BME688_IAQ_Accuracy"] = bsec_outputs[i].accuracy;
                                    } else if (bsec_outputs[i].sensor_id == BSEC_OUTPUT_CO2_EQUIVALENT) {
                                        payload["BME688_eCO2"] = bsec_outputs[i].signal;
                                    } else if (bsec_outputs[i].sensor_id == BSEC_OUTPUT_BREATH_VOC_EQUIVALENT) {
                                        payload["BME688_bVOC"] = bsec_outputs[i].signal;
                                    }
                                }
                            }
                        }
                    } else if (bsec_sensor_settings.op_mode == BME68X_SLEEP_MODE && bme_last_op_mode != BME68X_SLEEP_MODE) {
                        bme68x_set_op_mode(BME68X_SLEEP_MODE, &bme);
                        bme_last_op_mode = BME68X_SLEEP_MODE;
                    }
                }
            }
        }

        if (sgp41_ok && tcaselect(CH_SCD41_SGP41)) {
            uint16_t sv, sn;
            if (sgp41.measureRawSignals(0x8000, 0x6666, sv, sn) == 0) {
                voc_index = vocAlgo.process(sv);
                nox_index = noxAlgo.process(sn);
            }
        }

        if (bmv080_ok && tcaselect(CH_BMV080)) {
            bmv080_output_t out;
            if (bmv080.readSensor(&out)) {
                latest_pm25 = out.pm2_5_mass_concentration;
                latest_pm10 = out.pm10_mass_concentration;
            }
        }
    }

    // Odczyt asynchroniczny z UART (RG-15)
    while(RainSerial.available()) {
        char c = RainSerial.read();
        if (c == '\n') {
            if (rg15_buffer.indexOf("Acc") != -1) {
                int idx = rg15_buffer.indexOf("Acc");
                daily_rain = rg15_buffer.substring(idx + 4, rg15_buffer.indexOf(" mm", idx)).toFloat();
            }
            rg15_buffer = "";
        } else {
            rg15_buffer += c;
        }
    }
}

void executeSynchronousReadAndPush() {
    payload.clear();
    
    // 1. SHT45 kompensator (Najpierw temp i hum)
    if (sht45_ok && tcaselect(CH_SHT45)) {
        sensors_event_t h, t; if (!sht45.getEvent(&h, &t)) { ATLAS_LOG("[I2C ERROR] SHT45 (0x44) not responding!\n"); }
        if (t.temperature > -50 && t.temperature < 85) {
            latest_temp = t.temperature; latest_hum = h.relative_humidity;
            payload["SHT45_Temp"] = latest_temp; payload["SHT45_Hum"] = latest_hum;
        }
    }

    // 2. BMP585 (Zambretti)
    if (bmp585_ok && tcaselect(CH_BMP585)) {
        if (bmp585.performReading()) { } else { ATLAS_LOG("[I2C ERROR] BMP585 (0x46) read failed!\n"); } 
 if (bmp585_ok) {
            latest_press = bmp585.pressure / 100.0F; // Wyjście do hPa
            biometeo.updatePressureBuffer(latest_press);
            payload["BMP585_Pressure_hPa"] = latest_press;
        }
    }

    // 3. SCD41 (Odczyt wyniku Single Shot bez blokowania - wyzwalany wcześniej w pętli)
    if (scd41_ok && tcaselect(CH_SCD41_SGP41)) {
        uint16_t co2; float t, h;
        if (scd41.readMeasurement(co2, t, h) != 0) { ATLAS_LOG("[I2C ERROR] SCD41 (0x62) read error!\n"); } else if (co2 > 0 && co2 > 0) payload["SCD41_CO2_ppm"] = co2;
    }

    // 4. ILPS22QS 
    if (ilps_ok && tcaselect(CH_ILPS22QS)) {
        float p, t; ilps.GetPressure(&p); ilps.GetTemperature(&t);
        payload["ILPS22QS_Press_hPa"] = p;
    }



    // 6. AS3935 Lightning (CH4)
    if (as3935_ok && tcaselect(CH_AS3935)) {
        int dist = as3935.getLightningDistKm();
        if (dist != -1) payload["AS3935_Distance_Km"] = dist;
    }

    // Agregacja odpytywanych często w fast loop
    payload["BMV080_PM2_5"] = latest_pm25;
    payload["BMV080_PM10_0"] = latest_pm10;
    payload["SGP41_VOC_Index"] = voc_index;
    payload["SGP41_NOx_Index"] = nox_index;

    // Peryferia Główne (Rain)
    payload["RG15_Rain_mm"] = daily_rain;

    // --- MATH & FUSION ---
    if (latest_temp != 0.0 && latest_hum != 0.0) {
        payload["METEO_Dew_Point_C"] = biometeo.calcDewPoint(latest_temp, latest_hum);
        payload["METEO_Abs_Hum_g_m3"] = biometeo.calcAbsoluteHumidity(latest_temp, latest_hum);
        payload["METEO_Heat_Index"] = biometeo.calcHeatIndex(latest_temp, latest_hum);
    }
    if (latest_press > 0.0) {
        float slp = biometeo.calcSLP(latest_press, latest_temp, STATION_ALTITUDE);
        float d3h = biometeo.getPressureDelta3h(latest_press);
        payload["METEO_Sea_Level_Press_hPa"] = slp;
        payload["METEO_Pressure_3h_Delta"] = d3h;
        
        time_t now = time(nullptr);
        struct tm* ti = localtime(&now);
        if (now > 1600000000) payload["METEO_Zambretti_Forecast"] = biometeo.calcZambretti(slp, d3h, ti->tm_mon + 1);
    }
    payload["AIR_Smog_Index"] = biometeo.calcSmogIndex(latest_pm25, nox_index, latest_hum);

    // --- AIR QUALITY INDICES (Advanced Math) ---
    float aqi_pm25 = calcAQI_PM25(latest_pm25);
    int eaqi = calcEAQI(latest_pm25);
    float who_aqi_pct = calcWHO_AQI(latest_pm25, latest_pm10);
    float visibility_km = calcVisibility(latest_pm25, latest_pm10, latest_hum);
    float smog_idx = calcSmogIndex(latest_pm25, voc_index, latest_hum);
    float asthma_risk = calcAsthmaRisk(latest_pm25, voc_index, latest_temp, latest_hum);
    float resp_hazard = calcRespiratoryHazard(latest_pm25, voc_index / 100.0f, nox_index, latest_hum);

    payload["AIR_EPA_AQI"] = aqi_pm25;
    payload["AIR_EAQI_Index"] = eaqi;
    payload["AIR_WHO_AQI_Pct"] = who_aqi_pct;
    payload["AIR_Visibility_Km"] = visibility_km;
    payload["AIR_Smog_Index_Advanced"] = smog_idx;
    payload["MED_Asthma_Risk"] = asthma_risk;
    payload["MED_Respiratory_Hazard"] = resp_hazard;

    // Wypychka MQTT
    if (WiFi.status() == WL_CONNECTED) {
        if (!mqtt.connected()) {
            ATLAS_LOG("[MQTT] Lączenie z brokerem %s...\n", MQTT_SERVER);
            #ifdef MQTT_USER
            mqtt.connect(HOSTNAME, MQTT_USER, MQTT_PASS);
            #else
            mqtt.connect(HOSTNAME);
            #endif
        }
        if (mqtt.connected()) {
            // Zapis BSEC BaseLine do EEPROM co godzinę (60 cykli)
            static int bsec_save_timer = 0;
            if (++bsec_save_timer >= 60 && eeprom_ok && bme690_bsec_ready) {
                bsec_save_timer = 0;
                uint8_t state[BSEC_MAX_STATE_BLOB_SIZE] = {0};
                uint32_t len = 0;
                if (bsec_get_state(0, state, BSEC_MAX_STATE_BLOB_SIZE, bsec_work_buffer, sizeof(bsec_work_buffer), &len) == BSEC_OK) {
                    eeprom.write(0, state, BSEC_MAX_STATE_BLOB_SIZE);
                    ATLAS_LOG("[SYSTEM] BSEC3 State saved to EEPROM (0x50)\n");
                }
            }

            String out; serializeJson(payload, out);
            mqtt.publish("airsense/state", out.c_str());
            ATLAS_LOG("[TELEMETRY] Pushed 0x72 Environment Payload to MQTT.\n");
        } else {
            ATLAS_LOG("[TELEMETRY] MQTT offline. Payload dropped.\n");
        }
    } else {
        ATLAS_LOG("[TELEMETRY] Wi-Fi offline. Payload dropped.\n");
    }
}

// --- INTERWENCYJNE TRYBY PRACY (MAINTENANCE / RECOVERY) ---

void setupWiFiAndOTA() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSIDS[0], WIFI_PASSWORD);
    WiFi.setAutoReconnect(true);

    unsigned long startAttemptTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 10000) {
        delay(500);
        ATLAS_LOG(".");
    }
    if (WiFi.status() == WL_CONNECTED) ATLAS_LOG("\n[WIFI] Polaczone! IP: %s\n", WiFi.localIP().toString().c_str());
    else ATLAS_LOG("\n[WIFI] Timeout! Uruchamiam w trybie Offline.\n");

    ArduinoOTA.begin();
}

void checkRemoteMode() {
    if (WiFi.status() != WL_CONNECTED) return;
    HTTPClient http;
    http.begin(MODE_OVERRIDE_URL);
    int httpCode = http.GET();
    if (httpCode == 200) {
        String resp = http.getString();
        resp.trim();
        if (resp == "RECOVERY") currentMode = MODE_RECOVERY;
        else if (resp == "MAINTENANCE") currentMode = MODE_MAINTENANCE;
        else currentMode = MODE_CONTINUOUS;
    }
    http.end();
}

void exhaustivelyReadSensors() {
    readFastSensors(); // Obsługa PM, VOC, NOx, BSEC
    
    if (sht45_ok && tcaselect(CH_SHT45)) {
        sensors_event_t h, t; if (!sht45.getEvent(&h, &t)) { ATLAS_LOG("[I2C ERROR] SHT45 (0x44) not responding!\n"); }
        if (t.temperature > -50 && t.temperature < 85) {
            latest_temp = t.temperature; latest_hum = h.relative_humidity;
        }
    }
    if (bmp585_ok && tcaselect(CH_BMP585)) {
        if (bmp585.performReading()) { } else { ATLAS_LOG("[I2C ERROR] BMP585 (0x46) read failed!\n"); } 
 if (bmp585_ok) latest_press = bmp585.pressure / 100.0F;
    }
    if (scd41_ok && tcaselect(CH_SCD41_SGP41)) {
        uint16_t co2; float t, h;
        if (scd41.readMeasurement(co2, t, h) != 0) { ATLAS_LOG("[I2C ERROR] SCD41 (0x62) read error!\n"); } else if (co2 > 0 && co2 > 0) payload["SCD41_CO2_ppm"] = co2;
    }

    if (as3935_ok && tcaselect(CH_AS3935)) {
        int dist = as3935.getLightningDistKm();
        if (dist != -1) payload["AS3935_Distance_Km"] = dist;
    }
}







void printAsciiTable() {
    ATLAS_LOG("\n+----------------------------------------------------------------+\n");
    ATLAS_LOG("|                   A.T.L.A.S. SENSOR DASHBOARD                  |\n");
    ATLAS_LOG("+----------------------------------------------------------------+\n");
    ATLAS_LOG("| SHT45 (0x44):  %s | Temp: %5.2f C   Hum: %5.2f %%         |\n", sht45_ok ? "ONLINE " : "OFFLINE", latest_temp, latest_hum);
    ATLAS_LOG("| BMP585(0x46):  %s | Press: %7.2f hPa                       |\n", bmp585_ok ? "ONLINE " : "OFFLINE", latest_press);
    ATLAS_LOG("| BME690(0x77):  %s | IAQ: %3.0f   eCO2: %4.0f   bVOC: %3.2f     |\n", bme690_bsec_ready ? "READY  " : "OFFLINE", payload["BME688_IAQ"].as<float>(), payload["BME688_eCO2"].as<float>(), payload["BME688_bVOC"].as<float>());
    ATLAS_LOG("| SCD41 (0x62):  %s | CO2 NDIR: %4d ppm                     |\n", scd41_ok ? "ONLINE " : "OFFLINE", payload["SCD41_CO2_ppm"].as<int>());
    ATLAS_LOG("| SGP41 (0x59):  %s | VOC Index: %3d   NOx Index: %3d         |\n", sgp41_ok ? "ONLINE " : "OFFLINE", (int)voc_index, (int)nox_index);
    ATLAS_LOG("| BMV080(0x57):  %s | PM2.5: %5.2f     PM10: %5.2f           |\n", bmv080_ok ? "ONLINE " : "OFFLINE", latest_pm25, latest_pm10);
    ATLAS_LOG("| ILPS  (0x5C):  %s | HP Press: %7.2f hPa                    |\n", ilps_ok ? "ONLINE " : "OFFLINE", payload["ILPS22QS_Press_hPa"].as<float>());
    ATLAS_LOG("|----------------------------------------------------------------|\n");
    ATLAS_LOG("| PERIPHERALS    | Rain: %5.2f mm   WDT: ACTIVE (30s)        |\n", daily_rain);
    ATLAS_LOG("+----------------------------------------------------------------+\n");
}






void setup() {
    Serial.begin(115200);
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
    Wire.setClock(100000);
    Wire.setTimeOut(200);
    // --- WATCHDOG INIT ---
    esp_task_wdt_init(30, true); // 30 sekund timeout, panic enable
    esp_task_wdt_add(NULL);      // Dodaj glowny wattek do monitorowania
 // 200ms na rozciąganie zegara przez układy Boscha

#if ADMIN_MODE
    // Uruchamia pełny skan diagnostyczny przed wybudzeniem i alokacją reszty sprzętu
    runAdminI2CScan();
#endif

    setupWiFiAndOTA();
    checkRemoteMode();
    
    mqtt.setServer(MQTT_SERVER, MQTT_PORT);

    initSensors();
    
    // --- STARTUP HARDWARE AUDIT REPORT ---
    ATLAS_LOG("\n[SYSTEM] Performing initial sensor sweep...\n");
    exhaustivelyReadSensors();
    printAsciiTable();
    
    cycleStartTime = millis();
    currentPhase = PHASE_WARMUP;

    // --- SYSTEM & WWW INIT ---
    server.on("/", []() { server.send_P(200, "text/html; charset=utf-8", index_html); });
    server.on("/api", []() { 
        String s; serializeJson(payload, s); 
        server.send(200, "application/json; charset=utf-8", s); 
    });
    server.on("/wifi", []() { server.send(200, "text/plain", "OK"); });

    server.on("/logs", []() {
        String allLogs = "";
        int start = (logCount < MAX_LOG_LINES) ? 0 : logHead;
        for (int i = 0; i < logCount; i++) {
            allLogs += logBuffer[(start + i) % MAX_LOG_LINES];
        }
        server.send(200, "text/plain; charset=utf-8", allLogs);
    });

    server.on("/cmd", []() {
        if (server.hasArg("action")) {
            String act = server.arg("action");
            if (act == "reboot") { 
                server.send(200, "text/plain", "Rebooting..."); 
                delay(500); ESP.restart(); 
            } else if (act == "reset_i2c") { 
                Wire.end(); delay(100); Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN); Wire.setClock(100000);
                server.send(200, "text/plain", "I2C Reset OK");
            } else { 
                server.send(200, "text/plain", "Cmd received: " + act); 
            }
        } else {
            server.send(400, "text/plain", "Missing action parameter");
        }
    });

    server.on("/scan_json", []() {
        JsonDocument doc;
        JsonArray muxes = doc["muxes"].to<JsonArray>();
        JsonObject mObj = muxes.add<JsonObject>();
        mObj["addr"] = "0x72";
        JsonArray channels = mObj["channels"].to<JsonArray>();
        
        for (uint8_t ch = 0; ch < 8; ch++) {
            JsonObject chObj = channels.add<JsonObject>();
            chObj["ch"] = ch;
            if (tcaselect(ch)) {
                chObj["status"] = "OK";
                JsonArray devs = chObj["devices"].to<JsonArray>();
                for (uint8_t a = 1; a < 127; a++) {
                    if (a == MUX_ADDR) continue;
                    Wire.beginTransmission(a);
                    if (Wire.endTransmission() == 0) {
                        char addrStr[5]; sprintf(addrStr, "0x%02X", a);
                        devs.add(addrStr);
                    }
                }
            } else {
                chObj["status"] = "OFFLINE";
            }
        }
        String s; serializeJson(doc, s);
        server.send(200, "application/json; charset=utf-8", s);
    });

    // --- HTTP OTA FIRMWARE UPDATE ---
    server.on("/update", HTTP_POST, []() {
        server.sendHeader("Connection", "close");
        server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
        ESP.restart();
    }, []() {
        HTTPUpload& upload = server.upload();
        if (upload.status == UPLOAD_FILE_START) {
            ATLAS_LOG("[OTA] Upload started: %s\n", upload.filename.c_str());
            if (!Update.begin(UPDATE_SIZE_UNKNOWN)) Update.printError(Serial);
        } else if (upload.status == UPLOAD_FILE_WRITE) {
            if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) Update.printError(Serial);
        } else if (upload.status == UPLOAD_FILE_END) {
            if (Update.end(true)) {
                ATLAS_LOG("[OTA] Upload Success: %u B. Rebooting...\n", upload.totalSize);
            } else {
                Update.printError(Serial);
            }
        }
    });

    server.begin();
    
    ArduinoOTA.begin();
}


void handleSerialMenu() {
    if (Serial.available()) {
        char c = Serial.read();
        switch (c) {
            case 'm':
            case 'M':
                ATLAS_LOG("\n--- ATLAS SERVICE MENU ---\n");
                ATLAS_LOG("s - Run I2C Diagnostic Scan\n");
                ATLAS_LOG("r - Reboot System\n");
                ATLAS_LOG("i - Re-init Sensors\n");
                ATLAS_LOG("t - Print Status Table\n");
                ATLAS_LOG("l - Lock System Access\n");
                ATLAS_LOG("--------------------------\n");
                break;
            case 's':
            case 'S':
                runAdminI2CScan();
                break;
            case 'r':
            case 'R':
                ATLAS_LOG("[SYSTEM] Rebooting by user request...\n");
                delay(500);
                ESP.restart();
                break;
            case 'i':
            case 'I':
                initSensors();
                break;
            case 't':
            case 'T':
                printAsciiTable();
                break;
        }
    }
}
void loop() {
    handleSerialMenu();
    esp_task_wdt_reset(); 
    ArduinoOTA.handle(); 
    server.handleClient();

    if (WiFi.status() == WL_CONNECTED) { 
        server.handleClient(); 
        ArduinoOTA.handle(); 
    }

    if (currentMode == MODE_RECOVERY) {
        static bool recovery_started = false;
        if (!recovery_started) {
            ATLAS_LOG("\n[!!! EMERGENCY PULL DOWN !!!]\n");
            ATLAS_LOG("System entered RECOVERY mode. All scheduled routines suspended.\n");
            ATLAS_LOG("Waiting for manual OTA or HTTP intervention...\n");
            
            // W trybie RECOVERY wystawiamy maksymalnie uproszczony webserver do wgrywania OTA
            server.on("/", []() { server.send(200, "text/html; charset=utf-8", "<h2>ATLAS RECOVERY MODE</h2><p>System halted. Sensors suspended.</p><form method='POST' action='/update' enctype='multipart/form-data'><input type='file' name='update'><input type='submit' value='Flash Firmware'></form>"); });
            
            setupWiFiAndOTA();
            server.begin();
            recovery_started = true;
        }
        delay(100);
        return; // Blokada dalszego cyklu stacji
    }

    if (currentMode == MODE_MAINTENANCE) {
        static unsigned long lastMaintRead = 0;
        if (millis() - lastMaintRead > 3000) { // Refresh every 3 seconds
            lastMaintRead = millis();
            ATLAS_LOG("\n[DEBUG] Maintenance Mode Active - Refreshing Sensors...\n");
            exhaustivelyReadSensors();
            printAsciiTable();
        }
        delay(10);
        return;
    }

    static unsigned long lastModeCheck = 0;
    if (millis() - lastModeCheck > 60000) {
        lastModeCheck = millis();
        checkRemoteMode();
    }

    unsigned long currentMillis = millis();
    if (WiFi.status() == WL_CONNECTED) mqtt.loop();

    switch(currentPhase) {
        case PHASE_WAKEUP:
            cycleStartTime = currentMillis;
            scd41_triggered = false; // Reset flagi pomiaru CO2
            currentPhase = PHASE_WARMUP;
            break;
            
        case PHASE_WARMUP:
            // Czekamy 12 sekund na ustabilizowanie się czujników po resecie zasilania
            if (currentMillis - cycleStartTime >= 12000) {
                currentPhase = PHASE_READ;
                lastFastPollTime = currentMillis;
            }
            break;
            
        case PHASE_READ:
            readFastSensors();
            // Pełna minuta = uruchom ciężkie synchroniczne czytanie
            if (currentMillis - cycleStartTime >= 60000) {
                currentPhase = PHASE_PUSH_API;
            }
            break;
            
        case PHASE_PUSH_API:
            executeSynchronousReadAndPush();
            currentPhase = PHASE_SLEEP_WAIT;
            break;
            
        case PHASE_SLEEP_WAIT: {
            static bool sleep_cmd_sent = false;
            if (!sleep_cmd_sent) {
                if (bmv080_ok && tcaselect(CH_BMV080)) bmv080.setMode(0);
                if (scd41_ok && tcaselect(CH_SCD41_SGP41)) scd41.powerDown();
                
                sleep_cmd_sent = true;
                cycleStartTime = currentMillis; // Reset timera dla bezpiecznej pauzy
            }
            
            // Nieblokujące odczekanie 1000ms przed kolejnym cyklem (zamiast sztucznego delay)
            if (currentMillis - cycleStartTime >= 1000) {
                sleep_cmd_sent = false;
                currentPhase = PHASE_WAKEUP;
            }
            break;
        }
    }
}
           