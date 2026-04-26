/*
********************************************************************************
*                                                                              *
*           o/                                                                 *
*          /|                                                                  *
*          / \         _~^~^~_                                                 *
*      ===/   \======>`       `======         Gone Phishing...                 *
*                                             for environmental data!          *
*                                                                              *
*  +------------------------------------------------------------------------+  *
*  |  copyNotRight (c) 2026-4ever                                           |  *
*  |  PROUDLY PRESENTED BY: Silas Mariusz Grzybacz                          |  *
*  |  RELEASED FOR        : DevSpark & forum.qnap.net.pl                    |  *
*  +------------------------------------------------------------------------+  *
*                                                                              *
********************************************************************************
*/

/*
// ----- CRITICAL NOTE - DO NOT MODIFY ----- //
// CODE USES SOME NEW LIBRARIES OR NOT PUBLIC SDK
//
// ALWAYS USE:
// REFERENCE FILE: ../../ALL_LIBRARIES_BUNDLE.md
// (IF reference file is not up to date with libraries used,
// run: generate_bundles_v2.py)
// 
// DO NOT MODIFY DEFAULT LIBRARIES ../../libraries.def
// USE A COPY FROM ../../libraries
// ---------------------------------------- // 
*/

/*
##
##
##                  _____
##                 /   _)))
##                /   / 6 6
##               (   (    \         Hey babe, look at this damn cool
##               /  ,' __=          LogMan i found on the internet!
##              / __) /_  )        ______________________________________
##             ( /   ~  `(                 /
##              / / .) .) )     ../////      _____________
##             ( /\    (_       \   , ,     |        '\\\\\\
##              \| \  '  \       C    \     |        ' ____|_
##              :o /      \       \D_/      |   +    '||::::::
##               /\    _/  )   ___| (___    |        '||_____|
##               \/     )  |  /  \ ~ /  \   \'_______|_____|
##                |    /   | /'\  \_/ _' \  ___/____|___\___
##             ___|___/\___|(  <_ _____|/_\|    _    '  <<<:|
##            /              '-/ \________ |_________'___o_o|
##           /                 \~/::::::::|                  \
##          /                  '=========='                   \
##         /___________________________________________________\
##           |                                               |
##           |                                               |
##
*/

/* 
======================================================================================
[ A.T.L.A.S. CORE ARCHITECTURE & POWER MANAGEMENT MANIFESTO ]
WARNING: DO NOT MODIFY WITHOUT UNDERSTANDING THESE HARDWARE CONSTRAINTS!
1. SENSOR WARMUP RULES (dynamic_warmup_ms):
   - WAKEUP / BOOT / DEEP SLEEP: Minimum 30-45 seconds (Cold boot).
   - LIGHT SLEEP WAKEUP: 12 seconds.

2. SYSTEM MODES EXPLAINED:
   - [0] MODE_CONTINUOUS: ESP32 running 100%. No sleep.
   - [1] MODE_DEEP_SLEEP: Hardware deep sleep (5/10/60 min intervals). Max power save.
   - [2] MODE_LIGHT_SLEEP: CPU sleeps for 45s every minute. Laser/Geiger run in background.
//
// 3. I2C AUTO-HEALING & BATTERY/NIGHT OVERRIDES:
//    - System auto-forces Deep Sleep:
//         * At Night (23:00 - 06:00)
//         * When Battery < 70% (cascading 5, 10, 60 min intervals).
//    - I2C Auto-Healing: Hard reset of the bus if >3 sensors fail.
//    FROM V4 VERSION BATTERY & SLEEP MODE WILL BE REMOVED 
//
// ======================================================================================
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
#include "driver/pcnt.h" // --- DODANO BIBLIOTEKĘ DO OBSŁUGI SPRZĘTOWEGO PCNT ---

// --- SENSOR LIBRARIES ---
#include <bsec2.h>
#include "D:/Arduino/libraries/bsec_v3-3-0-0/release_bin/IAQ/config/bme690/bme690_iaq_33v_3s_4d/bsec_iaq.h"
#include <Adafruit_MS8607.h>   
#include "DFRobot_AS3935_I2C.h"
#include <SparkFun_AS7343.h> 
#include "Adafruit_VEML7700.h" 
#include "LTR390_DFR.h"        
#include <Adafruit_BME280.h>   
#include "SparkFun_BMV080_Arduino_Library.h" 
#include "DFRobot_INA219.h"    
#include <Adafruit_MAX1704X.h> 

// --- NEW SENSORS ---
#include "Adafruit_SHT4x.h"
#include <SensirionI2CSgp41.h>
#include <VOCGasIndexAlgorithm.h>
#include <NOxGasIndexAlgorithm.h>
#include <Adafruit_TSL2591.h>

// --- NEW LABORATORY SENSORS ---
#include <SensirionI2cScd4x.h>      // NDIR CO2
#include <Adafruit_BMP5xx.h>
#include <ILPS22QSSensor.h>         // ILPS22QST (STM32duino)
#include <SparkFun_AS7331.h>        // Medyczne UV
#include <Adafruit_TCS34725.h>      // Sensor Koloru RGB
#include <Adafruit_OPT4048.h>     // Sensor Koloru XYZ
#include "DFRobot_BMM350.h"         // BMM350 Precision Magnetometer
#include <Adafruit_LSM6DSOX.h>      // LSM6DSOX - 6-axis IMU (accel + gyro)
#include <Adafruit_LIS3MDL.h>       // LIS3MDL  - 3-axis Magnetometer

// ── ZMOD4510 Renesas SDK ── (NO2 + O3)
// Firmware SDK: D:\Arduino\libraries\Renesas-ZMOD4510-NO2_O3-Firmware
// precompiled=true; algo linked from lib/Espressif ESP/esp32s3/*.a
// HAL: built-in Arduino Wire HAL (hal/arduino/arduino.cpp)
#include "no2_o3-arduino.h"
#include "hal/arduino/arduino_hal.h"  // for HAL_Init()
#include <Adafruit_MLX90640.h>      // MLX90640 Kamera Termowizyjna


void bme688Callback(const bme68xData data, const bsecOutputs outputs, Bsec2 bsec);

// --- FORWARD DECLARATIONS ---
void ATLAS_LOG(const char* format, ...);
bool tcaselect(uint8_t mux_addr, uint8_t i);
void handleThermalCamera();
bool repairWiFi();

// ᗧ···ᗣ···ᗣ····ᗣ····ᗣ····ᗣ····ᗣ····ᗣ····ᗣ····ᗣ····ᗣ····ᗣ····ᗣ····ᗣ
// 1. NETWORK, API, GPS & MQTT CONFIGURATION
// ᗧ···ᗣ···ᗣ····ᗣ····ᗣ····ᗣ····ᗣ····ᗣ····ᗣ····ᗣ····ᗣ····ᗣ····ᗣ····ᗣ
static const char* WIFI_SSIDS[] = {
  "IoT.Zyndrama52.devspark.pl",
  "Zyndrama52.devspark.pl",
  "WiFi-7.Zyndrama52.devspark.pl",
  "WiFi-6.Zyndrama52.devspark.pl"
};

static constexpr size_t WIFI_SSID_COUNT = sizeof(WIFI_SSIDS) / sizeof(WIFI_SSIDS[0]);
static const char* WIFI_PASSWORD = "df65055501";

const char* API_URL         = "https://zyndrama52.devspark.pl/airsense/api.php";
const char* MODE_OVERRIDE_URL = "https://zyndrama52.devspark.pl/airsense/mode.php";

const char* HOSTNAME        = "AirSense-Node";

static const char* WU_STATION_ID = "INOWYS30";
static const char* WU_STATION_KEY = "4da3cc1270dc4d79a3cc1270dc8d79d7";

const char* MQTT_SERVER = "10.100.200.6";
const int MQTT_PORT = 1883;
const char* MQTT_USER = "atlas";
const char* MQTT_PASS = "axpl1029al";
const char* MQTT_TOPIC_STATE = "airsense/state";
static const char* TZ_INFO = "CET-1CEST,M3.5.0,M10.5.0/3";

// --- KONFIGURACJA QNAP SYSLOG (QuLog Center) ---
const char* SYSLOG_SERVER = "10.100.200.14";
const int SYSLOG_PORT = 1514;
static const bool ENABLE_SYSLOG = false; // DISABLED - set to true to enable syslog logging

WiFiUDP syslogUdp;

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
        if (testClient.connect("zyndrama52.devspark.pl", 80)) {
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

#define I2C_SDA_PIN 4   // [XIAO: D3] - Bezpieczny pin domeny RTC
#define I2C_SCL_PIN 5   // [XIAO: D4] - Bezpieczny pin domeny RTC

// --- DYNAMIC I2C MUX ROUTING ---
uint8_t MUX_BME280   = 0x70; uint8_t CH_BME280   = 2; // (0x70) - User: 0x76 @ CH2
uint8_t MUX_BMV080   = 0x72; uint8_t CH_BMV080   = 3; // (0x72)
uint8_t MUX_INA219   = 0x70; uint8_t CH_INA219   = 1; // (0x70) - User: 0x40 @ CH1
uint8_t MUX_MS8607   = 0x72; uint8_t CH_MS8607   = 6; // (0x72) - [DISABLED]
uint8_t MUX_MAX17048 = 0x70; uint8_t CH_MAX17048 = 0; // (0x70) - User: 0x36 @ CH0
uint8_t MUX_BME688   = 0x72; uint8_t CH_BME688   = 0; // BME688 AI Waveshare na CH0
uint8_t MUX_SHT45    = 0x72; uint8_t CH_SHT45    = 5; // (0x72)
uint8_t MUX_SGP41    = 0x72; uint8_t CH_SGP41    = 2; // (0x72) współdzielony z SCD41

// ======================== MUX 0x70 ========================
// CH0: MAX17048 (0x36) - LiPo Fuel Gauge
// CH1: INA219   (0x40) - Solar Monitor
// CH2: BME280   (0x76) - Enclosure/BMS
// CH3: EEPROM   (0x50) - BSEC State Storage (parallel)
// CH5: LSM6DSOX (0x6A) + LIS3MDL (0x1E) - IMU 6-axis + Mag (Zmiana wg notatki)
// CH6: TSL2591 (0x29) + OPT4048 (0x44) + TCS34725 (0x29) - Shared Optical
// CH7: [FREE]
// ======================== MUX 0x71 ========================
// CH0: AS3935   (0x03) - RF Lightning Strike Detector (Zmiana z 0x70 CH7)
// CH1: AS7343   (0x39) - 14ch Spectral (Zmiana z CH0)
// CH2: MLX90640 (0x33) - Thermal Camera
// CH3: VEML7700 (0x10) - Ambient Light
// CH4: LTR390   (0x53) - UV Index
// CH5: AS7331   (0x74) - UVA/B/C Radiometer (Zmiana z CH1)
// CH6: ILPS22QS (0x5C) - QVAR / Seismic / Static Pressure
// CH7: BMM350   (0x14) - Precision Magnetometer
// ======================== MUX 0x72 ========================
// CH0: BME688   (0x77) - AI Waveshare (Gas)
// CH1: ZMOD4510 (0x33) - NO2 + O3
// CH2: SCD41 (0x62) + SGP41 (0x59) - NDIR CO2 + VOC/NOx (Przelotka Qwiic)
// CH3: BMV080   (0x57) - Particulate Matter
// CH4: BME690   (0x77) - (chiński - problemy z odpaleniem)
// CH5: SHT45    (0x44) - Temp/Hum
// CH6: MS8607   (0x76) - [DISABLED]
// CH7: BMP585   (0x46) - Pressure/Altitude
// ===========================================================

uint8_t MUX_TSL2591  = 0x70; uint8_t CH_TSL2591  = 6; // shared optical CH6
uint8_t MUX_OPT4048  = 0x70; uint8_t CH_OPT4048  = 6; // shared optical CH6
uint8_t MUX_TCS34725 = 0x70; uint8_t CH_TCS34725 = 6; // shared optical CH6
uint8_t MUX_LSM6DSOX = 0x70; uint8_t CH_LSM6DSOX = 5; // Zmiana z 1 na 5
uint8_t MUX_LIS3MDL  = 0x70; uint8_t CH_LIS3MDL  = 5; // Zmiana z 1 na 5
uint8_t MUX_BMM350   = 0x71; uint8_t CH_BMM350   = 7; // (0x71)
uint8_t MUX_I2CMEM   = 0x00; uint8_t CH_I2CMEM   = 0; // Parallel on main bus
uint8_t MUX_AS7343   = 0x71; uint8_t CH_AS7343   = 1; // Zmiana z 0 na 1
uint8_t MUX_VEML     = 0x71; uint8_t CH_VEML     = 3; // Ambient Light
uint8_t MUX_LTR      = 0x71; uint8_t CH_LTR      = 4; // UV Index
uint8_t MUX_AS3935   = 0x71; uint8_t CH_AS3935   = 0; // Zmiana z 0x70 CH7 na 0x71 CH0
uint8_t MUX_SCD41    = 0x72; uint8_t CH_SCD41    = 2; // shared CH2
uint8_t MUX_BMP585   = 0x72; uint8_t CH_BMP585   = 7; // Pressure
uint8_t MUX_ILPS     = 0x71; uint8_t CH_ILPS     = 6; // shared CH6
uint8_t MUX_AS7331   = 0x71; uint8_t CH_AS7331   = 5; // Zmiana z 1 na 5
uint8_t MUX_MLX90640 = 0x71; uint8_t CH_MLX90640 = 2; // Thermal Camera
#define GEIGER_IRQ_PIN 6 // [XIAO: D5] - Pin RTC, używany do mikro-wybudzeń
#define AS3935_IRQ_PIN 7 // [XIAO: D8] - Pin RTC dla przerwań błyskawic

// Piny dla zewnętrznego UART
// W Seeed XIAO ESP32-S3 piny 17 i 18 NIE są wyprowadzone na złącza!
// Zmieniamy na dostępne sprzętowe TX/RX z płyty:
#define RAIN_RG15_RX 44 // [XIAO: D7] (UART RX)
#define RAIN_RG15_TX 43 // [XIAO: D6] (UART TX)

#define WDT_TIMEOUT_SECONDS 180 

enum SystemMode { MODE_CONTINUOUS, MODE_DEEP_SLEEP, MODE_LIGHT_SLEEP, MODE_MAINTENANCE, MODE_RECOVERY };

enum CyclePhase { PHASE_WAKEUP, PHASE_WARMUP, PHASE_READ, PHASE_PUSH_API, PHASE_SLEEP_WAIT };

RTC_DATA_ATTR unsigned long dynamic_warmup_ms = 45000;

const unsigned long CYCLE_DURATION_MS = 60000;
const unsigned long WARMUP_DURATION_MS = 12000; 
const unsigned long SENSOR_READ_INTERVAL_MS = 10000;

struct BME688_Ultimate_Config { 
    float sampleRate; 
    float tempOffset; 
    bool enableStateSave; 
    uint32_t stateSavePeriodMs; 
    const uint8_t* customAiProfile; 
};

const BME688_Ultimate_Config bme688Config = { 
    BSEC_SAMPLE_RATE_LP, 
    0.0f,                
    true, 
    (360 * 60 * 1000), 
    bsec_config_iaq
};

// TRYB PRACY POZYSKIWANY JEST ZDALNIE Z PLIKU MODE.PHP
// w locie
// DOCELOWO WDROŻYĆ KONTROLER
SystemMode currentMode = MODE_CONTINUOUS;
CyclePhase currentPhase = PHASE_WAKEUP;

// Power policy toggles
const bool ENABLE_NIGHT_AUTO_DEEP_SLEEP = false; // force Light Sleep -> Deep Sleep at night (23:00-06:00)

Preferences prefs;
WebServer server(80);
WiFiClient espClient;
PubSubClient mqtt(espClient);
std::vector<String> discovered_sensors;

unsigned long cycleStartTime = 0;
float baseline_voltage = 0.0f;
float baseline_charge_rate = 0.0f;
bool baseline_taken = false;

unsigned long lastReadTime = 0;
unsigned long lastFastPollTime = 0;
unsigned long lastBsecPollTime = 0;

// /| __________________________________________________________________ |/
// VIRTUAL MULTIMETER CONFIG
// \| __________________________________________________________________ |\
// DOCELOWO WDROŻYĆ PRZEZ KONTROLER
const float BATTERY_CAPACITY_MAH = 13200.0f; // 4x 3300mAh
const float SOLAR_PANEL_MAX_MW = 10000.0f;

float global_solar_mw = 0.0f;

// --- BMV080 TIMEOUT LOGIC ---
static int bmv_fail_counter = 0;

Bsec2 envSensor;                     bool bme688_ok = false;
SparkFunBMV080 bmv080;               bool bmv080_ok = false;
Adafruit_MAX17048 bat;               bool max17048_ok = false;
Adafruit_MS8607 ms8607;              bool ms8607_ok = false;
DFRobot_INA219_IIC ina219(&Wire, INA219_I2C_ADDRESS1); bool ina219_ok = false;
DFRobot_AS3935_I2C lightning((uint8_t)AS3935_IRQ_PIN, (uint8_t)0x03); bool as3935_ok = false;
Adafruit_BME280 bme280;              bool bme280_ok = false;
Adafruit_VEML7700 veml;              bool veml_ok = false;
LTR390_DFR ltr;                      bool ltr_ok = false;

// --- NEW SENSORS ---
Adafruit_SHT4x sht45;                bool sht45_ok = false;
SensirionI2CSgp41 sgp41;             bool sgp41_ok = false;
VOCGasIndexAlgorithm vocAlgorithm;
NOxGasIndexAlgorithm noxAlgorithm;
Adafruit_TSL2591 tsl2591 = Adafruit_TSL2591(2591); bool tsl2591_ok = false;
Adafruit_OPT4048 opt4048;                        bool opt4048_ok = false;
Adafruit_TCS34725 tcs34725 = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_50MS, TCS34725_GAIN_4X); bool tcs34725_ok = false;
bool i2c_mem_ok = false;

// --- NEW LABORATORY SENSORS OBJECTS ---
SensirionI2cScd4x scd41;             bool scd41_ok = false; bool scd41_triggered = false;
Adafruit_BMP5xx bmp585;              bool bmp585_ok = false;
ILPS22QSSensor ilps(&Wire);          bool ilps_ok = false;
SfeAS7331ArdI2C as7331;              bool as7331_ok = false;
SfeAS7343ArdI2C as7343;              bool as7343_ok = false;

Adafruit_MLX90640 mlx;               bool mlx90640_ok = false;

DFRobot_BMM350_I2C  bmm350(&Wire);            bool bmm350_ok = false;
Adafruit_LSM6DSOX   lsm6dsox;                 bool lsm6dsox_ok = false;
Adafruit_LIS3MDL    lis3mdl;                  bool lis3mdl_ok  = false;

// ── ZMOD4510 — Renesas Official SDK (NO2 + O3 + FAST_AQI + EPA_AQI) ──────
// I2C address: 0x33   Placement: any free MUX channel (assigned to 0x72 CH1)
// HAL ported to use Arduino Wire + ESP32 delay
static zmod4xxx_dev_t   zmod4510_dev;
static uint8_t          zmod4510_adc[ZMOD4510_ADC_DATA_LEN];
static uint8_t          zmod4510_prod[ZMOD4510_PROD_DATA_LEN];
static no2_o3_handle_t  zmod4510_algo_handle;
static no2_o3_results_t zmod4510_results;
static no2_o3_inputs_t  zmod4510_input;
static Interface_t      zmod4510_hal;
bool zmod4510_ok = false;
bool zmod4510_stabilizing = false;

// ZMOD4510 HAL is provided by the SDK's built-in Arduino HAL (hal/arduino/arduino.cpp)
// HAL_Init() populates Wire-based callbacks automatically.
uint8_t MUX_ZMOD4510 = 0x72; uint8_t CH_ZMOD4510 = 1; // (0x72) CH1 — I2C 0x33

// Struktura na PEŁNE 14 kanałów AS7343
struct AS7343_Data {
    uint16_t f1;    // 405 nm
    uint16_t f2;    // 425 nm
    uint16_t fz;    // 450 nm (NOWY)
    uint16_t f3;    // 475 nm
    uint16_t f4;    // 515 nm
    uint16_t f5;    // 550 nm
    uint16_t fy;    // 555 nm (NOWY)
    uint16_t fxl;   // 600 nm (NOWY)
    uint16_t f6;    // 640 nm
    uint16_t f7;    // 690 nm
    uint16_t f8;    // 745 nm
    uint16_t nir;   // 855 nm
    uint16_t clear; // Pełne spektrum
    uint16_t fd;    // Detekcja migotania (Flicker)
    uint16_t dark;  // Prąd ciemny (szum)
    uint32_t total_gain; // Zapis połączonego zysku (HW + SW)
} opt_data;

// --- RTC MEMORY FOR ALGORITHMS ---
RTC_DATA_ATTR float last_pressure_bmp585 = 0.0f;

// BIOMETEO RING BUFFER (Ciśnienie z ostatnich 3 godzin)
#define BIOMETEO_HIST_SIZE 18
RTC_DATA_ATTR float p_hist_hPa[BIOMETEO_HIST_SIZE] = {0};
RTC_DATA_ATTR uint64_t p_hist_ts[BIOMETEO_HIST_SIZE] = {0};
RTC_DATA_ATTR int p_hist_idx = 0;

// --- ANOMALY DETECTION BUFFERS (RTC MEMORY) ---
#define ANOMALY_HIST_SIZE 6
RTC_DATA_ATTR float anom_hist_temp[ANOMALY_HIST_SIZE] = {0};
RTC_DATA_ATTR float anom_hist_hum[ANOMALY_HIST_SIZE] = {0};
RTC_DATA_ATTR float anom_hist_pm25[ANOMALY_HIST_SIZE] = {0};
RTC_DATA_ATTR float anom_hist_voc[ANOMALY_HIST_SIZE] = {0};
RTC_DATA_ATTR float anom_hist_co2[ANOMALY_HIST_SIZE] = {0};
RTC_DATA_ATTR float anom_hist_press[ANOMALY_HIST_SIZE] = {0};
RTC_DATA_ATTR float anom_hist_cpm[ANOMALY_HIST_SIZE] = {0};
RTC_DATA_ATTR int anom_hist_idx = 0;
RTC_DATA_ATTR bool anom_hist_filled = false;

// --- SGP41 Global Buffers ---
int32_t current_voc_index = 0;
int32_t current_nox_index = 0;
uint16_t current_sraw_voc = 0;
uint16_t current_sraw_nox = 0;

// --- AS7343 Auto Gain Control (Saved in RTC) ---
RTC_DATA_ATTR uint8_t optics_gain_idx = 5;
// Wartości 0-13 odpowiadają fizycznym rejestrom wzmocnienia w układzie AS7343 (0.5x do 2048)
sfe_as7343_again_t as_gain_values[] = { 
    AGAIN_0_5, AGAIN_1, AGAIN_2, AGAIN_4, AGAIN_8, AGAIN_16, 
    AGAIN_32, AGAIN_64, AGAIN_128, AGAIN_256, AGAIN_512, AGAIN_1024, AGAIN_2048 
};

float as_gain_multipliers[] = { 
    0.5f, 1.0f, 2.0f, 4.0f, 8.0f, 16.0f, 
    32.0f, 64.0f, 128.0f, 256.0f, 512.0f, 1024.0f, 2048.0f 
};

// --- SYSTEM HEALTH & DIAGNOSTICS (RTC MEMORY) ---
RTC_DATA_ATTR time_t cold_boot_timestamp = 0;
RTC_DATA_ATTR time_t last_fault_timestamp = 0;
RTC_DATA_ATTR int previous_sensor_health_score = -1;

#define HEALTH_HIST_SIZE 15
RTC_DATA_ATTR int sensor_health_hist[HEALTH_HIST_SIZE] = {0};
RTC_DATA_ATTR int health_hist_idx = 0;
RTC_DATA_ATTR bool health_hist_filled = false;

volatile bool lightningTriggered = false;
uint32_t storm_strike_count = 0;
uint32_t last_strike_energy = 0;
uint8_t last_strike_dist = 0;

bool geiger_ok = false;
volatile uint32_t geiger_pulses = 0;

bool rg15_ok = false;

float wu_wind_speed = 0.0f;
float wu_wind_dir = 0.0f;
float wu_wind_gust = 0.0f;
bool wu_wind_fetched = false;
unsigned long wu_wind_last_fetch = 0;

void IRAM_ATTR detectLightning() { 
    lightningTriggered = true;
}

// --- NOWA FUNKCJA POBIERAJĄCA STAN PCNT GEIGERA ---
// (POZYSKIWANIE DANYCH PODCZAS DEEP SLEEP)
void updateGeigerPCNT() {
    if (!geiger_ok) return;
    int16_t pcnt_val = 0;
    pcnt_get_counter_value(PCNT_UNIT_0, &pcnt_val);
    if (pcnt_val > 0) {
        geiger_pulses += pcnt_val;
        pcnt_counter_clear(PCNT_UNIT_0);
    }
}

const int MAX_LOG_LINES = 120;
String logBuffer[MAX_LOG_LINES];
int logHead = 0;
int logCount = 0;
JsonDocument payload;

// -----------------------------------------------------------------------
// 3. HELPERS, LOGGING & MATH
// -----------------------------------------------------------------------

// --- STEROWNIK I2C EEPROM (Z obsługą stronicowania 32-bajtowego) ---
// wymagane aby nie zajechac pamieci NVS - potrzebne aby zapisywac stan
// uczenia maszynowego i kalibracji czujnikow np. BSEC (odniesienie do SDK)
void writeExtEEPROM(uint16_t memoryAddress, const uint8_t* data, size_t length) {
    if (!i2c_mem_ok) return;
    tcaselect(0x00, 0); // Izolacja szyny glownej
    const uint8_t PAGE_SIZE = 32;
    size_t written = 0;
    while (written < length) {
        Wire.beginTransmission(0x50);
        Wire.write((uint8_t)((memoryAddress + written) >> 8));   // MSB
        Wire.write((uint8_t)((memoryAddress + written) & 0xFF)); // LSB
        size_t spaceInPage = PAGE_SIZE - ((memoryAddress + written) % PAGE_SIZE);
        size_t toWrite = min(spaceInPage, length - written);
        for (size_t i = 0; i < toWrite; i++) Wire.write(data[written + i]);
        if (Wire.endTransmission() != 0) {
            ATLAS_LOG("[EEPROM] Write ERROR at addr %d\n", memoryAddress + written);
            return;
        }
        delay(6); // Cykl zapisu EEPROM (wymagane 5ms)
        written += toWrite;
    }
}

bool readExtEEPROM(uint16_t memoryAddress, uint8_t* data, size_t length) {
    if (!i2c_mem_ok) return false;
    tcaselect(0x00, 0); // Izolacja szyny glownej
    
    // Sygnatura weryfikacji: sprawdzamy czy na adresie 0 jest "magiczna liczba" BSEC
    // Żeby zapobiec wczytaniu śmieci z czystego EEPROM-a
    Wire.beginTransmission(0x50);
    Wire.write((uint8_t)(memoryAddress >> 8));
    Wire.write((uint8_t)(memoryAddress & 0xFF));
    if (Wire.endTransmission() != 0) return false;
    
    size_t readCount = 0;
    while (readCount < length) {
        size_t toRead = min((size_t)32, length - readCount);
        Wire.requestFrom((uint8_t)0x50, (uint8_t)toRead);
        int timeout = 100;
        while (Wire.available() < toRead && timeout-- > 0) delay(1);
        if (timeout <= 0) return false;
        for (size_t i = 0; i < toRead; i++) data[readCount++] = Wire.read();
    }
    return true;
}

// TODO: BEZ SYSLOG'a nie wiemy co sie dzieje
// TODO: WYPYCHAC BUFOR DO WLASNEGO KONTROLERA
void ATLAS_LOG(const char* format, ...) {
    char buf[1024];
    va_list arg;
    va_start(arg, format);
    vsnprintf(buf, sizeof(buf), format, arg);
    va_end(arg);

    // 1. Zapis do portu szeregowego i lokalnego bufora RAM (WebUI)
    Serial.print(buf);
    logBuffer[logHead] = String(buf);
    logHead = (logHead + 1) % MAX_LOG_LINES;
    if (logCount < MAX_LOG_LINES) logCount++;

    // 2. SYSLOG - tylko jeśli włączony
    if (ENABLE_SYSLOG && WiFi.status() == WL_CONNECTED) {
        String cleanMsg = String(buf);
        cleanMsg.trim();

        if (cleanMsg.length() > 0) {
            cleanMsg.replace("\n", " | ");

            String syslogPacket = "<14>ATLAS_Node AirSense: " + cleanMsg;
            syslogUdp.beginPacket(SYSLOG_SERVER, SYSLOG_PORT);
            syslogUdp.print(syslogPacket);
            syslogUdp.endPacket();
        }
    }
}

// TO JEST HEAVY CZESC, ROZWAZYC PELNE PRZENIESIENIE DO WRAPPERA
// BOSCH SDK: sdk_full_reference_dump.md
String getBMV080ErrorString(int code) {
    switch(code) {
        case 0:   return "E_BMV080_OK (Success)";
        case 100: return "E_BMV080_ERROR_NULLPTR";
        case 101: return "E_BMV080_ERROR_REG_ADDR";
        case 179: return "E_BMV080_ERROR_PARAM_LOCKED (Laser is running!)";
        case 208: return "E_BMV080_WARNING_FIFO_SW_BUFFER_SIZE";
        case 209: return "E_BMV080_WARNING_FIFO_HW_BUFFER_SIZE";
        case 215: return "E_BMV080_WARNING_FIFO_FULL";
        default:  return "UNKNOWN_BMV080_ERROR_CODE_" + String(code);
    }
}

void checkBMV080Status(int code, const char* context) {
    if (code != 0) ATLAS_LOG("[BMV080] ERROR during '%s': %s\n", context, getBMV080ErrorString(code).c_str());
}

String getBsecStatusString(int code) {
    switch(code) {
        case 0:   return "BSEC_OK";
        case -1:  return "BSEC_E_DOSTEPS_INVALIDINPUT";
        case -2:  return "BSEC_E_DOSTEPS_VALUELIMITS";
        case 12:  return "BSEC_I_SU_SUBSCRIBEDOUTPUTGATES";
        case 14:  return "BSEC_W_SU_SAMPLERATEMISMATCH (Timing desync!)";
        case 100: return "BSEC_W_SC_CALL_TIMING_VIOLATION (Harmless delay)";
        case -33: return "BSEC_E_CONFIG_FAIL";
        default:  return "UNKNOWN_CODE_" + String(code);
    }
}

void checkBsecStatus(Bsec2& bsec) {
    if (bsec.status < 0) {
        ATLAS_LOG("[BSEC2] CRITICAL ERROR: %s\n", getBsecStatusString(bsec.status).c_str());
    } else if (bsec.status > 0) {
        if (bsec.status != 14 && bsec.status != 100) {
            ATLAS_LOG("[BSEC2] WARNING/INFO: %s\n", getBsecStatusString(bsec.status).c_str());
        }
    }
}
// ---koniec HEAVY CZESCI

// --------------------- !!!! ----------------------------
// TODO: WPROWADZIC KASKADOWA SMART OBSLUGE MULTIPLEXERAMI
// TODO: WPROWADZIC KASKADOWA SMART OBSLUGE MULTIPLEXERAMI
// TODO: WPROWADZIC KASKADOWA SMART OBSLUGE MULTIPLEXERAMI
// TODO: WPROWADZIC KASKADOWA SMART OBSLUGE MULTIPLEXERAMI
// TODO: WPROWADZIC KASKADOWA SMART OBSLUGE MULTIPLEXERAMI
// TAK
// I SPIAC OPTYKE + STEVENSON PRZEZ JEDEN ROZDZDIELACZ
// TODO: WPROWADZIC KASKADOWA SMART OBSLUGE MULTIPLEXERAMI
// TODO: WPROWADZIC KASKADOWA SMART OBSLUGE MULTIPLEXERAMI
// TODO: WPROWADZIC KASKADOWA SMART OBSLUGE MULTIPLEXERAMI
// TODO: WPROWADZIC KASKADOWA SMART OBSLUGE MULTIPLEXERAMI
// TODO: WPROWADZIC KASKADOWA SMART OBSLUGE MULTIPLEXERAMI
// --------------------- !!!! ----------------------------
bool tcaselect(uint8_t mux_addr, uint8_t i) { 
    // Wyłącz inne multipleksery na magistrali (zapobiega kolizjom przy arch. równoległej)
    if (mux_addr != 0x70) { Wire.beginTransmission(0x70); Wire.write(0); Wire.endTransmission(); }
    if (mux_addr != 0x71) { Wire.beginTransmission(0x71); Wire.write(0); Wire.endTransmission(); }
    if (mux_addr != 0x72) { Wire.beginTransmission(0x72); Wire.write(0); Wire.endTransmission(); }
    
    if (mux_addr == 0x00) return true; // Szyna główna (I2C Mem) - muxy zablokowane
    if (i > 7) return false;

    Wire.beginTransmission(mux_addr);
    Wire.write(1 << i);
    if (Wire.endTransmission() != 0) return false; 
    delay(2);
    return true;
}
// --------------------- !!!! ----------------------------

// --- STEROWNIK I2C EEPROM (Z obsługą stronicowania 32-bajtowego) ---
// NIE GENERUJA PRZYPADKIEM ZBYT DUZEGO cpu% 
// I ROZWAZYC PRZENIESIENIE FORMUL NA KONTROLER DEDYKOWANY
// LUB HOME ASSISTANT
// ZLECIC AI WERYFIKACJE CZY TE OBLICZENIA NA FLOAT
// NIE GENERUJA PRZYPADKIEM ZBYT DUZEGO cpu% 
// I ROZWAZYC PRZENIESIENIE FORMUL NA KONTROLER DEDYKOWANY
// LUB HOME ASSISTANT
// --- METEO MATH HELPER FUNCTIONS ---
// [ZASADA DZIAŁANIA I OCZEKIWANE WYNIKI POMIARÓW]

const float STATION_ALTITUDE_METERS = 290.0f;

// 1. Punkt Rosy (Dew Point)
// Oblicza temperaturę, do której musi ochłodzić się powietrze, aby zawarta w nim wilgoć 
// uległa kondensacji (skropleniu). Wzór uproszczony (przybliżenie liniowe).
// Wynik: Jeśli DP blisko T (temperatury) - powstaje mgła/rosa. DP > 20°C oznacza uciążliwą duchotę.
float calcDewPoint(float t, float h) { return t - ((100.0f - h) / 5.0f); }

// 2. Wysokość Podstawy Chmur (Cloud Base)
// Reguła Henniga: gradient temperaturowy wilgotności wynosi około 1°C na 125m wznoszenia.
// Wynik: Wysokość w metrach nad poziomem gruntu, gdzie zaczynają się formować chmury cumulus. 
// Oczekiwane pomiary: 500-2000m w jasne dni, <200m podczas mglistej pogody.
float calcCloudBase(float t, float dp) { return (t - dp) * 125.0f; }

// 3. Wilgotność Absolutna (Absolute Humidity)
// Masa pary wodnej (w gramach) zawarta w 1 m^3 powietrza. Obliczana ze wzoru Clausiusa-Clapeyrona.
// Wynik: g/m³. Im wyższa wartość, tym bardziej nasycone jest powietrze. 
// >15 g/m³ odczuwalne jest jako ciężkie, tropikalne powietrze. Wartości <5 g/m³ wysuszają drogi oddechowe (częste zimą).
float calcAbsoluteHumidity(float t, float h) { return (6.112f * exp((17.67f * t)/(t + 243.5f)) * h * 2.1674f) / (273.15f + t); }

// 4. Ciśnienie zredukowane do poziomu morza (Sea Level Pressure)
// Koryguje ciśnienie lokalne (odczytane ze stacji) o jej wysokość, by umożliwić porównania z mapami synoptycznymi.
// Wynik: hPa (hektopaskale). Średnie to 1013 hPa. <1000 hPa to niż (zła pogoda, wiatr), >1020 hPa to wyż (słonecznie).
float calcSLP(float p, float t, float alt) { return p * pow((1.0f - (0.0065f * alt) / (t + 0.0065f * alt + 273.15f)), -5.257f); }

// 5. Indeks Ciepła (Heat Index / Odczuwalna)
// Łączy temperaturę z wilgotnością, aby określić odczucie cieplne człowieka. 
// Wynik: W °C. Uruchamia się faktycznie, gdy t > 27°C, ale działa tu ogólnie. 
// >32°C oznacza ostrożność, >41°C to niebezpieczeństwo udaru.
float calcHeatIndex(float t, float h) { return t + 0.33f * (h / 100.0f * 6.105f * exp(17.27f * t / (237.7f + t))) - 4.0f; }

// -----------------------------------------------------------------------
// NOAA "Feels Like" (Apparent Temperature) — official NWS formula
//
// Combines three regimes as used operationally by NOAA/NWS:
//   • T ≤ 10°C && wind ≥ 4.8 km/h → Wind Chill (NWS 2001 regression)
//   • T ≥ 27°C && RH ≥ 40%       → Heat Index (Rothfusz/NWS polynomial)
//   • Otherwise                   → Actual temperature (comfort zone)
//
// Wind Chill   source: https://www.weather.gov/mfl/windchill
// Heat Index   source: https://www.wpc.ncep.noaa.gov/html/heatindex_equation.shtml
//
// Inputs : temp_c  - air temperature          [°C]
//          hum_rh  - relative humidity        [%]
//          wind_kph- wind speed at 10 m agl   [km/h]  (0 = calm)
// Output : feels-like temperature             [°C]
// -----------------------------------------------------------------------
float calcFeelsLike(float temp_c, float hum_rh, float wind_kph) {

    // ---- 1. Wind Chill  (NWS 2001, valid T ≤ 10°C, V ≥ 4.8 km/h) ----
    if (temp_c <= 10.0f && wind_kph >= 4.8f) {
        // Formula published in Fahrenheit; convert, apply, convert back
        float tf = temp_c * 1.8f + 32.0f;
        float vf = wind_kph * 0.621371f;           // mph
        float wc_f = 35.74f + 0.6215f * tf
                   - 35.75f  * powf(vf, 0.16f)
                   + 0.4275f * tf * powf(vf, 0.16f);
        return (wc_f - 32.0f) / 1.8f;
    }

    // ---- 2. Heat Index  (Rothfusz full polynomial, NWS) ----------------
    if (temp_c >= 27.0f && hum_rh >= 40.0f) {
        float tf = temp_c * 1.8f + 32.0f;           // °F
        float rh = hum_rh;
        float hi_f =
            -42.379f
            +  2.04901523f  * tf
            + 10.14333127f  * rh
            -  0.22475541f  * tf * rh
            -  6.83783e-3f  * tf * tf
            -  5.481717e-2f * rh * rh
            +  1.22874e-3f  * tf * tf * rh
            +  8.5282e-4f   * tf * rh * rh
            -  1.99e-6f     * tf * tf * rh * rh;

        // NWS adjustments for edge cases
        if (rh < 13.0f && tf >= 80.0f && tf <= 112.0f)
            hi_f -= ((13.0f - rh) / 4.0f) * sqrtf((17.0f - fabsf(tf - 95.0f)) / 17.0f);
        else if (rh > 85.0f && tf >= 80.0f && tf <= 87.0f)
            hi_f += ((rh - 85.0f) / 10.0f) * ((87.0f - tf) / 5.0f);

        return (hi_f - 32.0f) / 1.8f;
    }

    // ---- 3. Comfort zone — actual temperature -------------------------
    return temp_c;
}

// 6. Gęstość Powietrza (Air Density)
// Korzysta z prawa gazu doskonałego. Ile waży kubik powietrza.
// Wynik: kg/m³. Standard na poz. morza to 1.225 kg/m³. Istotne w aerodynamice dronów i turbin wiatrowowych.
// Maleje wraz z wysokością i temperaturą (rzadsze powietrze).
float calcAirDensity(float p, float t) { return (p * 100.0f) / (287.05f * (t + 273.15f)); }

// 7. Temperatura termometru wilgotnego (Wet Bulb Temperature)
// Wzór empiryczny Stull'a. Symuluje najniższą temperaturę, jaką człowiek może osiągnąć przez odparowanie potu ze skóry.
// Wynik: W °C. Krytyczny wskaźnik przetrwania! Gdy WetBulb przekracza 35°C (np. w tropikach), ciało traci zdolność 
// chłodzenia się, co prowadzi do śmiertelnego przegrzania organizmu w ciągu kilku godzin.
float calcWetBulb(float t, float h) { 
    return t * atan(0.151977 * sqrt(h + 8.313659)) + atan(t + h) - atan(h - 1.676331) + 0.00391838 * pow(h, 1.5) * atan(0.023101 * h) - 4.686035;
}

// 8. Bezpieczny Czas Ekspozycji (Safe Sun Exposure)
// Wynik: W minutach, do wystąpienia rumienia skóry (dla cery europejskiej). Jeśli UVI to 10 (lato w południe), czas to ~20 minut.
float calcSafeSunExposure(float uvi) { 
    if(uvi <= 0.1f) return 999.0f; 
    return 200.0f / uvi;
}

// 9. Amerykański Indeks Jakości Powietrza EPA (AQI) z surowego PM2.5
// Liniowa interpolacja progowa, bazująca na normach środowiskowych US EPA.
// Wynik: 0-50 (Dobra), 51-100 (Umiarkowana), >150 (Niezdrowa).
int calcAQI_PM25(float pm25) {
    float c = pm25;
    if (c <= 12.0) return round((50.0 / 12.0) * c);
    if (c <= 35.4) return round((49.0 / 23.4) * (c - 12.1) + 51);
    if (c <= 55.4) return round((49.0 / 20.0) * (c - 35.5) + 101);
    if (c <= 150.4) return round((49.0 / 95.0) * (c - 55.5) + 151);
    if (c <= 250.4) return round((99.0 / 100.0) * (c - 150.5) + 201);
    if (c <= 350.4) return round((99.0 / 100.0) * (c - 250.5) + 301);
    return round((99.0 / 149.9) * (c - 350.5) + 401);
}

// --- METEO & MED MATH (FUSION) ---

// 10. Barometryczna korekta kosmicznego promieniowania tła (Galactic Cosmic Rays)
// Promieniowanie kosmiczne jest blokowane przez atmosferę. Gdy spada ciśnienie (jest "mniej" atmosfery nad nami),
// czujnik Geigera zarejestruje więcej rozpadów naturalnych. Odfiltrowujemy te fluktuacje.
float calcCorrectedGCR(float cpm, float pressure_hPa) {
    if (pressure_hPa < 800.0f || pressure_hPa > 1100.0f) return cpm; 
    return cpm * exp(0.0074f * (pressure_hPa - 1013.25f));
}

// 11. Wysokość gęstościowa (Density Altitude)
// Używana w lotnictwie. Określa jak rzadkie powietrze wydaje się być fizycznie.
// Wynik: metry. W upalny dzień ciśnienie jest niskie a powietrze rozrzedzone, samolot "czuje" się 
// jakby leciał np. 1000m wyżej niż w rzeczywistości, dając mniejszą siłę nośną.
float calcDensityAltitude(float pressure_hPa, float temp_c) {
    float pressureAltitude = 44330.0f * (1.0f - pow(pressure_hPa / 1013.25f, 0.190284f));
    float isaTemp = 15.0f - (0.0065f * pressureAltitude);
    return pressureAltitude + (36.2f * (temp_c - isaTemp));
}

// 12. Deficyt Ciśnienia Pary Wodnej (VPD - Vapor Pressure Deficit)
// Określa różnicę między ilością wilgoci w powietrzu, a tym ile powietrze mogłoby zatrzymać.
// Wynik: w kPa. Używane w inteligentnych szklarniach.
// 0.8 do 1.2 kPa to "sweet spot" dla roślin - idealna perspiracja ułatwiająca pobieranie soli mineralnych.
float calcVPD(float temp_c, float hum_rh) {
    float es = 0.61078f * exp((17.27f * temp_c) / (temp_c + 237.3f)); 
    float ea = es * (hum_rh / 100.0f);                                
    return es - ea;
}

// 13. Temperatura wrzenia wody (Boiling Point)
// Zmienia się z ciśnieniem środowiskowym (równanie Antoine'a / barometryczne).
// Wynik: °C. Zazwyczaj to ~100°C na poz. morza, ale spada w górach przez spadek ciśnienia atmosferycznego.
float calcBoilingPoint(float pressure_hPa) {
    return 1.0f / ((log(1013.25f / pressure_hPa) / 4030.182f) + (1.0f / 373.15f)) - 273.15f;
}

// 14. Oszacowanie zagrożenia dróg oddechowych z wielu sensorów (Respiratory Hazard)
// Indeks eksperymentalny łączący różne gazy i pyły z wilgotnością (powietrze wilgotne szybciej wkleja pył w płuca).
// Wynik: od 0.0 (powietrze alpejskie) do 10.0 (ekstremalnie duszące środowisko).
float calcRespiratoryHazard(float pm25, float bvoc_ppm, float nox_index, float hum) {
    float risk = (pm25 / 15.0f) + (bvoc_ppm * 2.0f) + (nox_index / 50.0f);
    if (hum < 30.0f) risk *= 1.2f; else if (hum > 80.0f) risk *= 1.1f;
    return min(risk, 10.0f);
}

// 15. True Airspeed (TAS) Estimation (Static proxy)
// Calculates theoretical TAS proxy based on local density altitude and sea level pressure.
// Wynik: knots (kt). Useful as an aviation-specific density metric. 
float calcTASProxy(float pressure_hPa, float temp_c) {
    // Standard temp at sea level = 15C. Temp lapse rate = -0.0065 C/m
    float slp = 1013.25f; // Standard Sea Level Pressure
    float lapse_rate = 0.0065f;
    float R = 287.05f;
    
    // Density calculation
    float density = (pressure_hPa * 100.0f) / (R * (temp_c + 273.15f));
    float standard_density = (slp * 100.0f) / (R * (15.0f + 273.15f));
    
    // Ratio of densities (sigma)
    float sigma = density / standard_density;
    
    // TAS proxy multiplier based on density
    // Assuming indicated airspeed of 100kts for proxy baseline
    float proxy_tas = 100.0f / sqrt(sigma);
    return proxy_tas;
}

// 16. Icing Risk Index
// Estimates the risk of structural icing based on temperature, humidity, and cloud base.
// Wynik: String description. Critical aviation parameter for small aircraft.
String calcIcingRisk(float temp_c, float hum_rh) {
    // Icing primarily occurs between 0C and -20C with visible moisture (high humidity)
    if (temp_c <= 0.0f && temp_c >= -20.0f && hum_rh >= 80.0f) {
        if (temp_c >= -10.0f) {
             return "High (Clear Ice)";
        } else {
             return "Moderate (Rime Ice)";
        }
    }
    return "None";
}

// 17. QFE (Field Elevation Pressure)
// Calculates the pressure at the station level, essentially what an altimeter set to 0 at the airfield would read.
// Wynik: hPa. 
float calcQFE(float station_pressure_hPa) {
     return station_pressure_hPa;
}

// --- NEW ADVANCED FORMULAS (ATLAS v2.2) ---

// 18. Zambretti Forecaster (Simplified)
// Provides a weather forecast based on pressure trend and current SLP.
// Returns a string describing the forecast.
String calcZambretti(float slp_hPa, float delta3h_hPa, int month) {
    bool rising = (delta3h_hPa > 0.5f);
    bool falling = (delta3h_hPa < -0.5f);
    bool summer = (month >= 4 && month <= 9);

    if (falling) {
        if (slp_hPa > 1020) return "Settled, worsening";
        if (slp_hPa > 1010) return "Changeable, rain later";
        if (slp_hPa > 1000) return "Rain, wind";
        return "Stormy, heavy rain";
    } else if (rising) {
        if (slp_hPa > 1020) return "Settled, fine";
        if (slp_hPa > 1010) return "Becoming fine";
        if (slp_hPa > 1000) return "Showery, improving";
        return "Unsettled, improving";
    } else {
        if (slp_hPa > 1015) return "Fine, stable";
        if (slp_hPa > 1005) return "Fairly fine";
        return "Showery, unsettled";
    }
}

// 19. Frost Risk Index (Frost_Warning_Pct)
// Estimates the risk of frost (szron/przymrozek) based on temperature and dew point.
float calcFrostRisk(float temp_c, float dew_point_c, int hour) {
    if (hour < 17 && hour > 9) return 0.0f; // Minimal risk during day
    if (temp_c > 10.0f) return 0.0f;
    
    float risk = 0.0f;
    if (dew_point_c < 0.0f) risk += 50.0f;
    if (temp_c < 4.0f) risk += (4.0f - temp_c) * 10.0f;
    if (dew_point_c < temp_c - 2.0f) risk += 20.0f;
    
    return min(risk, 100.0f);
}

// 20. Smog Index (WHO Weighted)
// Combines PM2.5, Humidity, and NOx for a more accurate health hazard index.
float calcSmogIndex(float pm25, float hum_rh, float nox_index) {
    float pm_factor = pm25 / 10.0f; // WHO limit is low
    float hum_multiplier = 1.0f + (max(0.0f, hum_rh - 70.0f) / 100.0f); // High humidity traps particles
    float nox_factor = nox_index / 50.0f;
    
    return min(10.0f, (pm_factor * hum_multiplier) + nox_factor);
}

// 21. Visibility Estimation (Koschmieder's Rule)
// Estimates visibility in kilometers based on PM particles and humidity.
float calcVisibility(float pm25, float pm10, float hum_rh) {
    float mass_conc = pm25 + (pm10 * 0.1f);
    if (mass_conc < 1.0f) mass_conc = 1.0f;
    
    // Extinction coefficient estimation (simplified)
    float b_ext = 0.03f * mass_conc; 
    if (hum_rh > 80.0f) b_ext *= (1.0f + pow((hum_rh - 80.0f) / 20.0f, 2));
    
    float vis = 3.912f / b_ext;
    return min(40.0f, vis); // Cap at 40km
}

// 22. Uproszczona Ewapotranspiracja (ET0 - Hargreaves)
// Estymuje parowanie wody z gleby (mm/dobę).
float calcET0(float temp_c, float lux, float temp_min, float temp_max) {
    float solar_radiation = lux * 0.0079f; // Lux to W/m2 approximation
    float ra = 15.0f; // Extraterrestrial radiation proxy (lat dependent, simplified)
    return 0.0023f * ra * (temp_c + 17.8f) * sqrt(temp_max - temp_min) * 0.408f;
}

// 23. Speed of Sound (Precise)
// Calculates speed of sound in m/s considering temperature and humidity.
float calcSpeedOfSound(float temp_c, float hum_rh, float press_hPa) {
    // Basic formula: c = 331.3 * sqrt(1 + T/273.15)
    // Plus humidity correction
    float c = 331.3f * sqrt(1.0f + temp_c / 273.15f);
    float hum_correction = 0.606f * (hum_rh / 100.0f) * (6.112f * exp(17.62f * temp_c / (243.12f + temp_c)) / press_hPa);
    return c + hum_correction;
}

// 24. Melanopic Lux (Melatonin Suppression Risk)
// Estimates melatonin suppression based on blue light spectral data.
float calcMelanopicLux(uint16_t f2_425, uint16_t fz_450, uint16_t f3_475) {
    // Weighted sum for blue-light sensitive melanopsin
    return (f2_425 * 0.2f + fz_450 * 1.0f + f3_475 * 0.5f) / 10.0f;
}

// 25. Mixing Ratio (w)
// Grams of water vapor per kilogram of dry air.
float calcMixingRatio(float temp_c, float hum_rh, float press_hPa) {
    float es = 6.112f * exp((17.67f * temp_c) / (temp_c + 243.5f));
    float e = es * (hum_rh / 100.0f);
    return 621.97f * e / (press_hPa - e);
}

// 26. European Air Quality Index (EAQI) - Based on PM2.5 and PM10
// Scale: 1 (Good) to 6 (Extremely Poor)
int calcEAQI(float pm25, float pm10) {
    int pm25_index = 1;
    if (pm25 > 10.0f) pm25_index = 2;
    if (pm25 > 20.0f) pm25_index = 3;
    if (pm25 > 25.0f) pm25_index = 4;
    if (pm25 > 50.0f) pm25_index = 5;
    if (pm25 > 75.0f) pm25_index = 6;

    int pm10_index = 1;
    if (pm10 > 20.0f) pm10_index = 2;
    if (pm10 > 40.0f) pm10_index = 3;
    if (pm10 > 50.0f) pm10_index = 4;
    if (pm10 > 100.0f) pm10_index = 5;
    if (pm10 > 150.0f) pm10_index = 6;

    return max(pm25_index, pm10_index);
}

// 27. WHO Air Quality Index (WHO 2021)
// Percent of the WHO 24-hour limit (15 ug/m3 for PM2.5, 45 ug/m3 for PM10)
float calcWHO_AQI(float pm25, float pm10) {
    float pm25_pct = (pm25 / 15.0f) * 100.0f;
    float pm10_pct = (pm10 / 45.0f) * 100.0f;
    return max(pm25_pct, pm10_pct);
}

// 30. Asthma Risk Index (0-10)
float calcAsthmaRisk(float pm25, float hum, float temp, float voc_idx) {
    float risk = (pm25 / 10.0f) + (voc_idx / 50.0f);
    if (hum > 80.0f || hum < 30.0f) risk *= 1.2f;
    if (temp < 0.0f || temp > 35.0f) risk *= 1.3f;
    return min(risk, 10.0f);
}

// 31. Stroke Risk (Rapid pressure drop proxy)
String calcStrokeRisk(float delta3h_hPa) {
    if (delta3h_hPa <= -3.0f) return "High (Rapid Drop)";
    if (delta3h_hPa >= 3.0f) return "Elevated (Rapid Rise)";
    return "Low (Stable)";
}

// 32. Wildfire Risk Index (0-100)
float calcFireRisk(float temp_c, float hum_rh, float vpd) {
    if (temp_c < 10.0f) return 0.0f;
    float risk = (temp_c * 2.0f) - hum_rh + (vpd * 10.0f);
    return max(0.0f, min(risk, 100.0f));
}

// 28. AQI Warning String
String getAQIWarningLevel(int eaqi) {
    switch(eaqi) {
        case 1: return "Good";
        case 2: return "Fair";
        case 3: return "Moderate";
        case 4: return "Poor";
        case 5: return "Very Poor";
        case 6: return "Extremely Poor";
        default: return "Unknown";
    }
}

// 29. Vitamin D Synthesis Timer (Skin Type 2)
// Estimates minutes to synthesize 1000 IU of Vitamin D.
float calcVitDTime(float uvi) {
    if (uvi < 0.5f) return 0.0f;
    return 200.0f / uvi; 
}

// =============================================================================
// BIOMETEOROLOGICAL PAIN MODELS — v2.0
// =============================================================================
// Scientific basis:
//   Migraine:      Rains & Penzien (2005) Headache, Prince et al. (2004),
//                  Lipton et al. (2004), Kimoto et al. (2011 — pressure triggers)
//   Rheumatic:     Strusberg et al. (2002) J Rheumatol, Jamison et al. (1995),
//                  Aikman (1997) Soc Sci Med
//   Barometric PI: Shutty et al. (1992) Pain — pressure change & chronic pain
// =============================================================================

// ─────────────────────────────────────────────────────────────────────────────
// Migraine Risk Index (0–10)
//
// Key barometric triggers (all documented, peer-reviewed):
//  1. Pressure DROP > 1.5 hPa / 3h  (strongest trigger, Rains 2005)
//  2. Absolute SLP < 1005 hPa        (low-pressure environment)
//  3. Rapid instantaneous dp/dt      (storm approach)
//  4. High humidity > 80%            (mucous membrane pressure)
//  5. Chinook/Föhn pattern:          warm + dry + fast drop (Prince 2004)
//  6. UV photosensitivity risk        (high UVI during migraine predisposition)
//  7. Temperature drop > 5°C / 3h    (vasoconstriction trigger, Lipton 2004)
//  8. High VOC / poor IAQ            (chemical trigger, odour sensitivity)
//
// Inputs: delta3h [hPa/3h], dp_dt [hPa/min], slp [hPa], temp [°C],
//         hum [%], uvi [-], voc_idx [-], temp_delta [°C/3h]
// ─────────────────────────────────────────────────────────────────────────────
float calcMigraineRisk(float delta3h, float dp_dt, float slp,
                       float temp, float hum, float uvi,
                       float voc_idx, float temp_delta3h) {
    float risk = 0.0f;

    // 1. Pressure drop (strongest trigger) — bidirectional but drop >> rise
    if      (delta3h <= -5.0f)  risk += 4.0f;
    else if (delta3h <= -3.0f)  risk += 3.0f;
    else if (delta3h <= -1.5f)  risk += 2.0f;
    else if (delta3h >=  5.0f)  risk += 1.0f; // pressure RISE is also a mild trigger

    // 2. Absolute low pressure environment
    if      (slp < 998.0f)      risk += 2.0f;
    else if (slp < 1002.0f)     risk += 1.0f;
    else if (slp < 1005.0f)     risk += 0.5f;

    // 3. Rapid instantaneous dp/dt (approaching storm)
    if (fabsf(dp_dt) > 1.5f)    risk += 1.0f;

    // 4. High humidity (mucous pressure, sinus congestion)
    if      (hum > 90.0f)       risk += 1.5f;
    else if (hum > 80.0f)       risk += 0.8f;

    // 5. Chinook/Föhn: drop + warm + dry (very potent Alberta migraine pattern)
    if (delta3h <= -2.0f && temp > 15.0f && hum < 40.0f)  risk += 1.5f;

    // 6. UV photosensitivity (UVI > 6 worsens photophobia pre-attack)
    if      (uvi > 8.0f)        risk += 1.0f;
    else if (uvi > 5.0f)        risk += 0.5f;

    // 7. Rapid temperature drop (vasomotor instability)
    if      (temp_delta3h < -5.0f)  risk += 1.0f;
    else if (temp_delta3h < -2.5f)  risk += 0.5f;

    // 8. High indoor VOC / chemical triggers
    if      (voc_idx > 200.0f)  risk += 1.0f;
    else if (voc_idx > 100.0f)  risk += 0.5f;

    return min(10.0f, risk);
}

String getMigraineCategory(float risk) {
    if (risk < 1.0f)  return "Minimal";
    if (risk < 3.0f)  return "Low";
    if (risk < 5.0f)  return "Moderate";
    if (risk < 7.0f)  return "High";
    return "Very High";
}

// ─────────────────────────────────────────────────────────────────────────────
// Rheumatological / Arthritis Pain Risk Index (0–10)
//
// Key triggers (Strusberg 2002, Aikman 1997, Jamison 1995):
//  1. Cold temperature < 10°C                 (synovial fluid thickening)
//  2. Cold AND damp: T < 15°C + RH > 75%     (strongest combo, Strusberg)
//  3. Pressure RISE > 1.5 hPa / 3h           (inward tissue displacement)
//  4. Pressure FALL (both directions painful, rise slightly worse)
//  5. High absolute humidity (swelling, lymph pressure)
//  6. Absolute low barometric pressure (joint distension)
//  7. Wind chill effect (exposed joint exposure)
//  8. Dew point < 5°C + rapid temp fall (ice, fog, frozen ground cues)
// ─────────────────────────────────────────────────────────────────────────────
float calcRheumaticRisk(float delta3h, float slp, float temp_c,
                        float hum_rh, float abs_hum, float wind_chill_c,
                        float dew_c) {
    float risk = 0.0f;

    // 1. Cold temperature — synovial fluid viscosity increases
    if      (temp_c < -5.0f)    risk += 3.0f;
    else if (temp_c <  2.0f)    risk += 2.0f;
    else if (temp_c < 10.0f)    risk += 1.5f;
    else if (temp_c < 16.0f)    risk += 0.5f;

    // 2. Cold AND damp — the classic Strusberg pattern (strongest evidence)
    if (temp_c < 15.0f && hum_rh > 75.0f)  risk += 2.5f;
    if (temp_c < 10.0f && hum_rh > 85.0f)  risk += 1.0f; // extra cold-damp bonus

    // 3. Pressure RISE — tissues pushed inward against swollen joints
    if      (delta3h >=  4.0f)  risk += 2.5f;
    else if (delta3h >=  2.0f)  risk += 1.5f;
    else if (delta3h >=  1.0f)  risk += 0.8f;

    // 4. Pressure FALL — also painful but less so than rise
    if      (delta3h <= -4.0f)  risk += 1.5f;
    else if (delta3h <= -2.0f)  risk += 0.8f;

    // 5. High absolute humidity (lymphatic pressure, tissue swelling)
    if      (abs_hum > 15.0f)   risk += 1.0f;
    else if (abs_hum > 12.0f)   risk += 0.5f;

    // 6. Low absolute pressure (joint cavity distension)
    if      (slp < 995.0f)      risk += 2.0f;
    else if (slp < 1000.0f)     risk += 1.0f;

    // 7. Effective wind chill (peripheral joint exposure)
    if      (wind_chill_c < -10.0f) risk += 1.5f;
    else if (wind_chill_c <   0.0f) risk += 0.8f;

    // 8. Near-freezing dew point (frost, icy conditions)
    if (dew_c < 2.0f && temp_c < 8.0f)  risk += 0.5f;

    return min(10.0f, risk);
}

String getRheumaticCategory(float risk) {
    if (risk < 1.5f)  return "Minimal";
    if (risk < 3.5f)  return "Low";
    if (risk < 5.5f)  return "Moderate";
    if (risk < 7.5f)  return "High";
    return "Very High";
}

// ─────────────────────────────────────────────────────────────────────────────
// Barometric Pain Index (BPI) — Shutty et al. (1992)
// Combined chronic pain sensitivity, pressure change as primary driver.
// Used in pain management centres; correlates with fibromyalgia flares.
// Scale: 0–10, where 5 = typical quiet-weather baseline
// ─────────────────────────────────────────────────────────────────────────────
float calcBarometricPainIndex(float delta3h, float dp_dt, float slp) {
    float bpi = 5.0f; // neutral baseline
    // Deviation from 1013.25 hPa
    float abs_dev = fabsf(slp - 1013.25f);
    bpi += abs_dev * 0.08f;
    // Rate of change (any direction)
    bpi += fabsf(delta3h) * 0.4f;
    bpi += fabsf(dp_dt)   * 2.0f;
    return min(10.0f, max(0.0f, bpi));
}

// ─────────────────────────────────────────────────────────────────────────────
// Sinus Pressure / Congestion Risk (0–10)
// Triggered by: rapid pressure drop, high humidity,
// cold air inhalation, high allergen conditions
// ─────────────────────────────────────────────────────────────────────────────
float calcSinusRisk(float delta3h, float hum_rh, float temp_c, float pm25) {
    float risk = 0.0f;
    if (delta3h < -2.0f)         risk += 3.0f;
    else if (delta3h < -1.0f)    risk += 1.5f;
    if (hum_rh > 85.0f)          risk += 2.0f;
    else if (hum_rh > 75.0f)     risk += 1.0f;
    if (temp_c < 5.0f)           risk += 1.5f; // cold air → mucous irritation
    else if (temp_c < 12.0f)     risk += 0.8f;
    if (pm25 > 25.0f)            risk += 1.5f;
    else if (pm25 > 15.0f)       risk += 0.8f;
    return min(10.0f, risk);
}

// ─────────────────────────────────────────────────────────────────────────────
// Overall Biometeorological Sensitivity Score (0–10)
// Weighted average of all pain indices — useful for "bad weather day" alert.
// ─────────────────────────────────────────────────────────────────────────────
float calcBiometeoScore(float migraine_risk, float rheumatic_risk,
                        float bpi, float sinus_risk) {
    return min(10.0f,
        (migraine_risk  * 0.30f +
         rheumatic_risk * 0.35f +
         bpi            * 0.20f +
         sinus_risk     * 0.15f));
}

// ─────────────────────────────────────────────────────────────────────────────
/*
 * Wind chill is the perceived decrease in air temperature felt by the body
 * on exposed skin due to the flow of air. This function uses the formula
 * standardized by Environment Canada, the US National Weather Service, and the UK Met Office.
 * The formula is only applied for temperatures at or below 10.0°C and wind speeds
 * above 3.0 kph. Outside these ranges, the actual temperature is returned.
 *
 * @param temp_c The ambient air temperature in degrees Celsius.
 * @param wind_kph The wind speed in kilometers per hour.
 * @return The calculated wind chill temperature in degrees Celsius. If conditions
 *         are outside the valid range, returns the original temperature.
 *
 * @example
 * float windChill = calcWindChill(-5.0f, 20.0f); // Result will be approx -12.6°C
 */
// ─────────────────────────────────────────────────────────────────────────────
float calcWindChill(float temp_c, float wind_kph) {
    if (temp_c > 10.0f || wind_kph < 3.0f) return temp_c;
    return 13.12f + 0.6215f * temp_c - 11.37f * pow(wind_kph, 0.16f) + 0.3965f * temp_c * pow(wind_kph, 0.16f);
}

// ─────────────────────────────────────────────────────────────────────────────
/**
 * @brief Calculates a simplified Heat Stress Index.
 *
 * This function provides an estimation of heat stress. The calculation is based on
 * temperature and relative humidity, with adjustments made for specific ranges.
 * The index is not calculated for temperatures below 20.0°C.
 *
 * @note This appears to be a custom or simplified model, not a standard index like WBGT or Humidex.
 *
 * @param temp_c The ambient air temperature in degrees Celsius.
 * @param hum_rh The relative humidity in percent (e.g., 65.0 for 65%).
 * @return A numerical value representing the heat stress level. Returns 0.0f for temperatures below 20°C.
 *
 * @example
 * float heatStress = calcHeatStressIndex(30.0f, 70.0f);
 */// ─────────────────────────────────────────────────────────────────────────────
float calcHeatStressIndex(float temp_c, float hum_rh) {
    if (temp_c < 20.0f) return 0.0f;
    float hi = 0.0f;
    if (temp_c >= 27.0f && temp_c <= 43.0f) {
        hi = -8.694f + 0.6215f * temp_c + (0.3965f * temp_c * (hum_rh / 100.0f));
        if (temp_c >= 27.0f && temp_c < 32.0f && hum_rh >= 50.0f) hi += (0.1f * temp_c);
        else if (temp_c >= 32.0f && temp_c < 41.0f && hum_rh >= 40.0f) hi += (0.05f * temp_c) - 3.0f;
        else if (temp_c > 41.0f) hi += (0.05f * temp_c) - 8.0f;
    }
    return hi;
}

// ─────────────────────────────────────────────────────────────────────────────
/**
 * @brief Calculates the Apparent Temperature ("Feels Like").
 *
 * This function combines heat index and wind chill effects to provide a single
 * "feels like" temperature. It uses different formulas based on whether the
 * conditions are hot or cold.
 *
 * @param temp_c The ambient air temperature in degrees Celsius.
 * @param hum_rh The relative humidity in percent (e.g., 65.0 for 65%).
 * @param wind_kph The wind speed in kilometers per hour.
 * @return The apparent temperature in degrees Celsius.
 *
 * @example
 * float feelsLikeHot = calcApparentTemp(32.0f, 60.0f, 5.0f);
 * float feelsLikeCold = calcApparentTemp(-5.0f, 50.0f, 25.0f);
 */
// ─────────────────────────────────────────────────────────────────────────────
float calcApparentTemp(float temp_c, float hum_rh, float wind_kph) {
    float at = temp_c;
    if (temp_c >= 27.0f) {
        at = -8.784694756f + 1.61139411f * temp_c + 0.133442976f * hum_rh + -0.01068573536f * temp_c * hum_rh;
    } else if (temp_c <= 10.0f && wind_kph > 0) {
        at = 13.126010f + 0.6215f * temp_c - 13.9441065f * pow(wind_kph + 1.0f, 0.16f) + 0.489681199f * temp_c * pow(wind_kph + 1.0f, 0.16f);
    }
    return at;
}

// ─────────────────────────────────────────────────────────────────────────────
/**
 * @brief Calculates a categorical Discomfort Index (DI) based on Thom's DI.
 *
 * This function first calculates a raw discomfort value using a formula derived
 * from Thom's Discomfort Index. It then classifies this value into one of four
 * comfort levels.
 *
 * @param temp_c The ambient air temperature in degrees Celsius.
 * @param hum_rh The relative humidity in percent (e.g., 65.0 for 65%).
 * @return A comfort category:
 *         - 0: No discomfort (DI < 21)
 *         - 1: Less than 50% of population feels discomfort (21 <= DI < 24)
 *         - 2: More than 50% of population feels discomfort (24 <= DI < 27)
 *         - 3: Most of the population suffers discomfort (DI >= 27)
 *
 * @example
 * float discomfortCategory = calcDiscomfortIndex(28.0f, 75.0f); // Likely returns 2 or 3
 */
// ─────────────────────────────────────────────────────────────────────────────
float calcDiscomfortIndex(float temp_c, float hum_rh) {
    float di = (1.8f * temp_c - 0.55f * (1 - hum_rh/100.0f) * (1.8f * temp_c - 26.0f) + 26.0f) - 25.5555556f;
    if (di < 21.0f) return 0;
    else if (di < 24.0f) return 1;
    else if (di < 27.0f) return 2;
    return 3;
}

// ─────────────────────────────────────────────────────────────────────────────
/**
 * @brief Calculates Thom's Index of Discomfort.
 *
 * This function calculates a simplified version of Thom's Index.
 * @note The formula mixes Celsius input (`temp_c`) with Fahrenheit-based constants (61, 68),
 * which may lead to unexpected results. It should be used with caution.
 *
 * @param temp_c The ambient air temperature in degrees Celsius.
 * @param hum_rh The relative humidity in percent (e.g., 65.0 for 65%).
 * @return The calculated Thom's Index value.
 *
 * @example
 * float thomIndex = calcThomIndex(25.0f, 60.0f);
 */
// ─────────────────────────────────────────────────────────────────────────────
float calcThomIndex(float temp_c, float hum_rh) {
    float d = 0.5f * (temp_c + 61.0f + ((temp_c - 68.0f) * (1 - hum_rh/100.0f)));
    return d;
}

// ─────────────────────────────────────────────────────────────────────────────
/**
 * @brief Calculates an estimated Wet Bulb Globe Temperature (WBGT).
 *
 * WBGT is a measure of heat stress in direct sunlight, which takes into account
 * temperature, humidity, wind speed, sun angle, and cloud cover. This function
 * provides a simplified estimation.
 *
 * @param temp_c The ambient air temperature in degrees Celsius.
 * @param hum_rh The relative humidity in percent (e.g., 65.0 for 65%).
 * @param globetemp_c The Black Globe Temperature in degrees Celsius. This requires a special sensor.
 * @return The estimated Wet Bulb Globe Temperature in degrees Celsius.
 *
 * @example
 * float wbgt = calcWetBulbGlobeTemp(30.0f, 50.0f, 35.0f);
 */
// ─────────────────────────────────────────────────────────────────────────────
float calcWetBulbGlobeTemp(float temp_c, float hum_rh, float globetemp_c) {
    float wb = 0.683f * temp_c + 0.693f * globetemp_c + 0.033f * (hum_rh / 100.0f) * globetemp_c - 0.358f * temp_c * (hum_rh / 100.0f) + 9.44f;
    return wb;
}

// ─────────────────────────────────────────────────────────────────────────────
/**
 * @brief Categorizes the UV Index (UVI) into a numerical risk level.
 *
 * This function maps the continuous UVI value to a discrete risk category
 * based on the WHO standard scale.
 *
 * @param uvi The UV Index value.
 * @return An integer representing the risk level:
 *         - 0: Low (UVI < 3)
 *         - 1: Moderate (3 <= UVI < 6)
 *         - 2: High (6 <= UVI < 8)
 *         - 3: Very High (8 <= UVI < 11)
 *         - 4: Extreme (UVI >= 11)
 *
 * @example
 * float riskLevel = calcUVRisk(7.5f); // Returns 2 (High)
 */
// ─────────────────────────────────────────────────────────────────────────────
float calcUVRisk(float uvi) {
    if (uvi < 3) return 0;
    else if (uvi < 6) return 1;
    else if (uvi < 8) return 2;
    else if (uvi < 11) return 3;
    return 4;
}

// ─────────────────────────────────────────────────────────────────────────────
/**
 * @brief Gets the human-readable name for a UV Index (UVI) value.
 *
 * Complements `calcUVRisk` by providing a string representation of the risk level.
 *
 * @param uvi The UV Index value.
 * @return A string describing the UV risk level ("Low", "Moderate", "High", "Very High", "Extreme").
 *
 * @example
 * String riskName = getUVRiskName(7.5f); // Returns "High"
 */
// ─────────────────────────────────────────────────────────────────────────────
String getUVRiskName(float uvi) {
    if (uvi < 3) return "Low";
    else if (uvi < 6) return "Moderate";
    else if (uvi < 8) return "High";
    else if (uvi < 11) return "Very High";
    return "Extreme";
}

// ─────────────────────────────────────────────────────────────────────────────
/**
 * @brief Calculates the Canadian Humidex (Humidity Index).
 *
 * The Humidex combines temperature and humidity into a single value to reflect
 * the perceived temperature. It is not calculated for temperatures below 15.0°C.
 *
 * @param temp_c The ambient air temperature in degrees Celsius.
 * @param hum_rh The relative humidity in percent (e.g., 65.0 for 65%).
 * @return The Humidex value. If temperature is below 15°C, returns the original temperature.
 *
 * @example
 * float humidex = calcHumidex(28.0f, 70.0f); // Result will be approx 37
 */
// ─────────────────────────────────────────────────────────────────────────────
float calcHumidex(float temp_c, float hum_rh) {
    if (temp_c < 15.0f) return temp_c;
    float dew = temp_c - ((100.0f - hum_rh) / 5.0f);
    float exp_val = exp(5417.753f * (1.0f / 273.15f - 1.0f / (273.15f + dew)));
    float h = temp_c + 0.5555f * (6.11f * exp_val - 10.0f);
    return h;
}

// ─────────────────────────────────────────────────────────────────────────────
/**
 * @brief Attempts to calculate a thermal comfort metric, likely related to PMV.
 *
 * This function initializes parameters used in the Predicted Mean Vote (PMV) model,
 * such as clothing insulation (clo) and metabolic rate (met). However, the current
 * implementation appears incomplete and only returns an intermediate value from the
 * full PMV calculation.
 *
 * @note This function is likely a work-in-progress or a simplified stub.
 *       The returned value is not the complete PMV index.
 *
 * @param temp_c The ambient air temperature in degrees Celsius.
 * @param hum_rh The relative humidity in percent (e.g., 65.0 for 65%).
 * @return An intermediate value from a thermal comfort calculation.
 */
// ─────────────────────────────────────────────────────────────────────────────
float calcComfortZone(float temp_c, float hum_rh) {
    float pmv = 0.0f;
    float clo = 0.5f;
    float met = 1.0f;
    float ta = temp_c;
    float tr = temp_c;
    float va = 0.1f;
    float rh = hum_rh;
    float pa = rh * 10 * exp(16.6536f - 4030.183f / (ta + 235.0f));
    float icl = 0.155f * clo;
    float mw = met * 58.15f;
    float fac = (va * 8.3f) / (0.155f * clo);
    float fcl = (fac > 0) ? (1 + 0.5f * fac) : 1.0f;
    float ts = 0.303f * exp(0.036f * (273 + ta)) + 1;
    float pmva = mw - 0.000001f * fac * (pa - rh);
    return pmva;
}

// ─────────────────────────────────────────────────────────────────────────────
// Temperature history for 3h delta (used by migraine model)
// ─────────────────────────────────────────────────────────────────────────────
#define TEMP_HIST_SIZE 18  // same cadence as pressure (~10 min steps)
static float t_hist_C[TEMP_HIST_SIZE]   = {0.0f};
static uint64_t t_hist_ts[TEMP_HIST_SIZE] = {0};
static int   t_hist_idx = 0;

// ─────────────────────────────────────────────────────────────────────────────
// ─────────────────────────────────────────────────────────────────────────────
void updateBiometeoBuffer(float current_p) {
    uint64_t now_sec = time(nullptr);
    if (now_sec < 1600000000) return; 
    if (now_sec - p_hist_ts[p_hist_idx] >= 600) {
        p_hist_idx = (p_hist_idx + 1) % BIOMETEO_HIST_SIZE;
        p_hist_hPa[p_hist_idx] = current_p;
        p_hist_ts[p_hist_idx] = now_sec;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// ─────────────────────────────────────────────────────────────────────────────
void updateTempHistory(float current_t) {
    uint64_t now_sec = time(nullptr);
    if (now_sec < 1600000000) return;
    if (now_sec - t_hist_ts[t_hist_idx] >= 600) {
        t_hist_idx = (t_hist_idx + 1) % TEMP_HIST_SIZE;
        t_hist_C[t_hist_idx]  = current_t;
        t_hist_ts[t_hist_idx] = now_sec;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// ─────────────────────────────────────────────────────────────────────────────
float getPressureDelta3h(float current_p) {
    uint64_t now_sec = time(nullptr); float oldest_p = current_p;
    uint64_t target_ts = now_sec - (3 * 3600); 
    for (int i = 0; i < BIOMETEO_HIST_SIZE; i++) {
        if (p_hist_ts[i] > 0 && abs((long long)p_hist_ts[i] - (long long)target_ts) < 1800) {
            oldest_p = p_hist_hPa[i]; break;
        }
    }
    return current_p - oldest_p;
}

// ─────────────────────────────────────────────────────────────────────────────
// ─────────────────────────────────────────────────────────────────────────────
float getTempDelta3h(float current_t) {
    uint64_t now_sec = time(nullptr); float oldest_t = current_t;
    uint64_t target_ts = now_sec - (3 * 3600);
    for (int i = 0; i < TEMP_HIST_SIZE; i++) {
        if (t_hist_ts[i] > 0 && abs((long long)t_hist_ts[i] - (long long)target_ts) < 1800) {
            oldest_t = t_hist_C[i]; break;
        }
    }
    return current_t - oldest_t;
}

// ============================================================================
// GAS IDENTIFICATION SYSTEM - Pattern-based gas type interpretation
// ============================================================================
// Based on BME688 Gas Estimates + SGP41 VOC/NOx + SCD41 CO2 patterns

String identifyGasPattern(float gas_est_1, float gas_est_2, float gas_est_3, float gas_est_4, 
                         float voc_idx, float nox_idx, float co2_ppm, float iaq) {
    // Gas Estimate 1-4 from BSEC are AI-based, not specific gas identification
    // But patterns can indicate broad categories
    
    bool high_voc = voc_idx > 100;
    bool high_nox = nox_idx > 10;
    bool high_co2 = co2_ppm > 1000;
    bool high_iaq = iaq > 150;
    
    // High gas estimates suggest specific patterns
    bool high_ge1 = gas_est_1 > 50;  // Often correlates to alcohols/aldehyde
    bool high_ge2 = gas_est_2 > 50;  // Often correlates to VOCs
    bool high_ge3 = gas_est_3 > 50;  // Often correlates to urban pollution
    bool high_ge4 = gas_est_4 > 50;  // Often correlates to other compounds
    
    String result = "Clean";
    
    if (high_nox && high_voc) {
        result = "Vehicle Exhaust";
    } else if (high_ge1 && high_co2) {
        result = "Alcohol/Respiration";
    } else if (high_ge3 && high_nox) {
        result = "Industrial Pollution";
    } else if (high_iaq && high_voc) {
        result = "Poor Ventilation";
    } else if (high_co2 && !high_voc) {
        result = "Crowded/Respiration";
    } else if (high_voc && nox_idx < 5) {
        result = "Cleaning Products";
    } else if (high_ge4 && high_iaq) {
        result = "Building Materials";
    } else if (nox_idx > 5) {
        result = "Combustion Present";
    }
    
    return result;
}

String getGasTypeCategory(float gas_est_1, float gas_est_2, float gas_est_3, float gas_est_4) {
    float max_est = max(max(gas_est_1, gas_est_2), max(gas_est_3, gas_est_4));
    float threshold = max_est * 0.8f;
    
    if (max_est < 20) return "Trace Gases";
    if (gas_est_1 > threshold) return "Alcohols/Aldehydes";
    if (gas_est_2 > threshold) return "VOCs";
    if (gas_est_3 > threshold) return "Pollutants";
    return "Mixed Compounds";
}

int calcToxicityRisk(float voc_idx, float nox_idx, float co2_ppm, float iaq) {
    int risk = 0;
    if (voc_idx > 200) risk += 2;
    else if (voc_idx > 100) risk += 1;
    
    if (nox_idx > 20) risk += 2;
    else if (nox_idx > 10) risk += 1;
    
    if (co2_ppm > 2000) risk += 3;
    else if (co2_ppm > 1000) risk += 1;
    
    if (iaq > 200) risk += 2;
    else if (iaq > 100) risk += 1;
    
    return min(risk, 10);
}

String getToxicityName(int risk) {
    if (risk <= 1) return "Safe";
    if (risk <= 3) return "Low Risk";
    if (risk <= 5) return "Moderate";
    if (risk <= 7) return "High Risk";
    return "Dangerous";
}

float calcIndoorAirQualityScore(float iaq, float voc_idx, float co2_ppm, float pm25) {
    float score = 100.0f;
    
    // IAQ penalty (0-200 scale inverted)
    if (iaq > 0) score -= (iaq / 2.0f);
    
    // VOC penalty
    if (voc_idx > 0) score -= (voc_idx / 10.0f);
    
    // CO2 penalty (>800 is baseline)
    if (co2_ppm > 800) score -= ((co2_ppm - 800) / 100.0f);
    
    // PM2.5 penalty
    if (pm25 > 0) score -= (pm25 * 0.5f);
    
    return max(0.0f, min(100.0f, score));
}

String predictAirQualityTrend(float iaq, float voc_idx, float prev_iaq, float prev_voc) {
    if (prev_iaq < 0 || prev_voc < 0) return "Unknown";
    
    float iaq_delta = iaq - prev_iaq;
    float voc_delta = voc_idx - prev_voc;
    
    if (iaq_delta < -10 && voc_delta < -20) return "Improving";
    if (iaq_delta > 10 && voc_delta > 20) return "Worsening";
    if (iaq_delta > 5 || voc_delta > 10) return "Degrading";
    return "Stable";
}

// =============================================================================
// ADVANCED VIRTUAL SENSORS — BLOCK 2
// =============================================================================

// ─────────────────────────────────────────────────────────────────────────────
// METEO: Global Horizontal Irradiance (GHI) in W/m²
// Converts photometric lux (visible spectrum) to radiometric irradiance.
// Formula: Constant 120 lux ≈ 1 W/m² for solar spectrum (ISO 9060).
// UVI correction term improves accuracy under high-UV conditions.
// Valid range: 0–1200 W/m²  (1000 W/m² = peak summer noon clear sky)
// ─────────────────────────────────────────────────────────────────────────────
float calcGHI_Wm2(float lux, float uvi) {
    if (lux < 1.0f) return 0.0f;
    float ghi = lux / 120.0f;
    // UVI carries significant near-UV energy not captured by broadband lux
    if (uvi > 0.0f) ghi += uvi * 3.2f;
    return min(ghi, 1400.0f);
}

// ─────────────────────────────────────────────────────────────────────────────
// METEO: Lifting Condensation Level (LCL) — Bolton (1980) formula
// The altitude where a rising air parcel reaches saturation — more precise
// than the simple Henning cloud-base formula (which we keep for comparison).
// Formula: LCL_K = 1/(1/(Td-56) + ln(T_K/Td_K)/800) + 56  [Bolton 1980]
//          LCL_m = (T_K - LCL_K) * 125                    [dry-adiabatic lapse]
// Output: LCL altitude in metres AGL
// ─────────────────────────────────────────────────────────────────────────────
float calcLCL_m(float temp_c, float dew_c) {
    float T_K  = temp_c + 273.15f;
    float Td_K = dew_c  + 273.15f;
    if (Td_K <= 56.0f || T_K <= 0.0f) return 0.0f;
    float lcl_K = 1.0f / (1.0f / (Td_K - 56.0f) + logf(T_K / Td_K) / 800.0f) + 56.0f;
    return (T_K - lcl_K) * 125.0f;
}

// ─────────────────────────────────────────────────────────────────────────────
// METEO: Equivalent Potential Temperature θe (Bolton 1980)
// Conserved quantity for moist-adiabatic processes; negative lapse rate in θe
// indicates CONDITIONAL INSTABILITY → thunderstorm/convection potential.
// Input:  temp_c [°C], hum_rh [%], press_hPa [hPa]
// Output: θe [Kelvin]
// Typical: 290–370 K; rapid rise toward 360 K signals deep convection risk.
// ─────────────────────────────────────────────────────────────────────────────
float calcEquivPotentialTemp(float temp_c, float hum_rh, float press_hPa) {
    if (press_hPa < 1.0f) return 0.0f;
    float T_K = temp_c + 273.15f;
    // Saturation vapour pressure (Tetens) [hPa]
    float es  = 6.112f * expf(17.67f * temp_c / (temp_c + 243.5f));
    float e   = (hum_rh / 100.0f) * es;
    // Mixing ratio [kg/kg]
    float r   = 0.622f * e / (press_hPa - e);
    // Potential temperature [K]
    float theta = T_K * powf(1000.0f / press_hPa, 0.2854f);
    // Latent heat / Cp correction (Bolton Eq. 38)
    float Lv = 2500000.0f; // J/kg
    float Cp = 1005.0f;    // J/(kg·K)
    return theta * expf((Lv * r) / (Cp * T_K));
}

// ─────────────────────────────────────────────────────────────────────────────
// METEO: Precipitable Water (PW) — column water vapour estimate
// Integrates absolute humidity over an estimated boundary-layer depth.
// Formula: PW_mm ≈ abs_humidity [g/m³] × scale_height_m / 1000
// Scale height ≈ 2500 m (tropical moist BL) … 1500 m (dry/cold)
// Derived from GPS-PW literature; accuracy ~±25% without radiosonde data.
// ─────────────────────────────────────────────────────────────────────────────
float calcPrecipitableWater(float abs_hum_gm3, float temp_c) {
    // Scale height varies with temperature (warmer = taller moist BL)
    float scale_h = 1500.0f + max(0.0f, temp_c) * 40.0f;
    return (abs_hum_gm3 * scale_h) / 1000.0f; // mm
}

// ─────────────────────────────────────────────────────────────────────────────
// ASTRO: Aerosol Optical Depth (AOD) proxy
// Uses ratio of global irradiance to top-of-atmosphere estimate to derive
// column extinction by aerosols (dust, haze, smoke).
// Only valid during daylight hours (lux > 5000).
// Interpretation: AOD < 0.1 = very clean; 0.1-0.3 = modest haze; > 0.4 = heavy
// ─────────────────────────────────────────────────────────────────────────────
float calcAOD_proxy(float ghi_wm2, float uvi, float press_hPa) {
    if (ghi_wm2 < 50.0f || uvi < 0.5f) return -1.0f; // not valid at night/twilight
    // Clear-sky GHI estimate using simplified Bird model: approx 950 * sin(elev)
    // We proxy solar elevation from UVI (UVI ≈ 10 × sin(theta) × E0 × T_atm)
    float sin_elev = min(1.0f, uvi / 10.0f);
    if (sin_elev < 0.05f) return -1.0f;
    float clear_sky_ghi = 950.0f * sin_elev * (press_hPa / 1013.25f);
    float transmittance  = min(1.0f, ghi_wm2 / clear_sky_ghi);
    if (transmittance <= 0.01f) return 3.0f; // near-zero transmittance
    // Rayleigh AOD for this elevation (pressure-corrected)
    float aod_rayleigh = 0.1f * (press_hPa / 1013.25f);
    float total_od = -logf(transmittance) / max(0.01f, 1.0f / sin_elev);
    return max(0.0f, total_od - aod_rayleigh);
}

// ─────────────────────────────────────────────────────────────────────────────
// SPACE WEATHER: Ozone Column Proxy (Dobson Units) from AS7331 UVB/UVA
// Ground-level UVB is strongly absorbed by stratospheric ozone.
// High UVB/UVA ratio → reduced ozone shield → more UVB penetrating.
// Calibration:  ~ 325 DU baseline; each 1 DU drop ≈ 1.5% UVB increase.
// Formula based on differential optical depth between 305 nm and 365 nm.
// Needs: AS7331 UVB [µW/cm²], UVA [µW/cm²], solar elevation proxy (from UVI).
// Valid only during daylight (uva > 5 µW/cm²).
// ─────────────────────────────────────────────────────────────────────────────
float calcOzoneProxy_DU(float uvb_uwcm2, float uva_uwcm2, float uvi) {
    if (uva_uwcm2 < 5.0f || uvb_uwcm2 < 0.01f) return -1.0f; // night/invalid
    float sin_elev = min(1.0f, max(0.01f, uvi / 10.0f));
    float ratio    = uvb_uwcm2 / uva_uwcm2;
    // Higher ratio → less ozone attenuation of UVB
    // Empirical: ratio ≈ 0.004 at 300 DU, increases ~0.0012/DU below that
    // DU = 300 + (ratio - 0.004) * (-1000) / sin_elev (air mass correction)
    float du = 300.0f - ((ratio - 0.004f) * 1000.0f / sin_elev);
    return max(150.0f, min(500.0f, du));
}

// ─────────────────────────────────────────────────────────────────────────────
// SPACE WEATHER: Local K-index proxy from magnetometer deviation
// The official K-index measures 3-hour magnetic disturbance from a quiet
// baseline at a reference observatory.  We implement a LOCAL proxy:
//   • Maintain a 60-minute rolling average of BMM350 total field |B| as baseline
//   • Compute deviation |B_now - B_quiet| in nanoTesla
//   • Map to K-scale (linear approximation for mid-latitude, ~50°N geographic)
//
// NOTE: Official worldwide K-indexes come from NOAA SWPC; this is a local
// proxy only — useful for detecting regional disturbances and aurora alerts.
//
// Mid-latitude K-to-nT conversion (IAGA standard, 50°N):
//   K0=0–5 nT, K1=5–10, K2=10–20, K3=20–40, K4=40–70, K5=70–120, K6=120–200,
//   K7=200–330, K8=330–500, K9=500+
// ─────────────────────────────────────────────────────────────────────────────

// Rolling baseline storage (updated once per cycle when field is quiet)
#define MAG_BASELINE_SIZE 12        // 12 cycles × 5 min = 60 min baseline window
static float mag_baseline_buf[MAG_BASELINE_SIZE] = {0};
static int   mag_baseline_idx = 0;
static int   mag_baseline_cnt = 0;
static float mag_baseline_val = 0.0f; // current quiet-day estimate

void updateMagBaseline(float total_field_uT) {
    if (total_field_uT < 1.0f) return; // ignore sensor errors
    mag_baseline_buf[mag_baseline_idx] = total_field_uT;
    mag_baseline_idx = (mag_baseline_idx + 1) % MAG_BASELINE_SIZE;
    if (mag_baseline_cnt < MAG_BASELINE_SIZE) mag_baseline_cnt++;
    // Baseline = median of buffer (robust against spikes)
    float sorted[MAG_BASELINE_SIZE];
    memcpy(sorted, mag_baseline_buf, sizeof(sorted));
    // Simple insertion sort on small buffer
    for (int i = 1; i < mag_baseline_cnt; i++) {
        float key = sorted[i]; int j = i - 1;
        while (j >= 0 && sorted[j] > key) { sorted[j+1] = sorted[j]; j--; }
        sorted[j+1] = key;
    }
    mag_baseline_val = sorted[mag_baseline_cnt / 2]; // median
}

int calcKIndexProxy(float total_field_uT) {
    if (mag_baseline_cnt < 3 || mag_baseline_val <= 0.0f) return -1; // not ready
    float dev_nT = fabsf(total_field_uT - mag_baseline_val) * 1000.0f; // µT → nT
    if (dev_nT <   5.0f) return 0;
    if (dev_nT <  10.0f) return 1;
    if (dev_nT <  20.0f) return 2;
    if (dev_nT <  40.0f) return 3;
    if (dev_nT <  70.0f) return 4;
    if (dev_nT < 120.0f) return 5;
    if (dev_nT < 200.0f) return 6;
    if (dev_nT < 330.0f) return 7;
    if (dev_nT < 500.0f) return 8;
    return 9;
}

// ─────────────────────────────────────────────────────────────────────────────
// SPACE WEATHER: Aurora Borealis probability
// Station latitude: ~49.6°N (Nowy Sącz).  At this latitude aurora is rare but
// theoretically visible during major geomagnetic storms (Kp 8-9).
//
// Geomagnetic latitude ≈ geographic + 11.5° → ~61°N effective
// Aurora equatorward boundary (km) vs Kp: boundary_lat ≈ 67 - 2.5*(Kp-3)
// Probability = 0% at Kp < 7, scales to 98% at Kp = 9
// ─────────────────────────────────────────────────────────────────────────────
float calcAuroraProbability(int k_proxy, float geomag_lat_N) {
    if (k_proxy < 0) return 0.0f; // baseline not ready
    // Geomagnetic boundary for aurora oval equatorward edge
    float boundary = 67.0f - 2.5f * max(0, k_proxy - 3);
    if (geomag_lat_N < boundary) return 0.0f;
    float overshoot = geomag_lat_N - boundary;
    float prob = min(98.0f, overshoot * 15.0f); // ~7% per degree past boundary
    return prob;
}

// ─────────────────────────────────────────────────────────────────────────────
// MEDICAL: UTCI — Universal Thermal Climate Index (Fiala/Bröde 2012)
// ISO/TR 11079, WMO-recommended heat stress standard since 2015.
// This is a truncated polynomial approximation (Bröde 2009, 40 dominant terms)
// accurate to ±1°C for: -20°C ≤ Ta ≤ 50°C, Va 0.5–17 m/s, RH 0–100%
// Input: Ta [°C], RH [%], Va [m/s], Tmrt [°C] — if no globe sensor: Tmrt = Ta
// Output: UTCI [°C]   Stress categories:
//   > 46: Extreme heat    38-46: Very strong heat   32-38: Strong heat
//   26-32: Moderate heat   9-26: No thermal stress   0-9: Slight cold
//   -13 to 0: Moderate cold   -27 to -13: Strong cold   <-27: Extreme cold
// ─────────────────────────────────────────────────────────────────────────────
float calcUTCI(float Ta, float RH, float wind_kph, float ghi_wm2) {
    // Mean radiant temperature estimation (no globe thermometer)
    // In sunshine: Tmrt ≈ Ta + alpha*GHI where alpha ~0.08 for outdoors
    float Tmrt = Ta + (ghi_wm2 > 50.0f ? 0.08f * ghi_wm2 : 0.0f);
    float D    = Tmrt - Ta;      // Tmrt delta
    float Va   = max(0.5f, wind_kph / 3.6f); // km/h → m/s, min 0.5
    float Pa   = RH / 100.0f *
                 expf(17.62f * Ta / (243.12f + Ta)) / 100.0f; // vapour pres kPa

    // Bröde (2009) polynomial — 40-term subset with |coeff| > 5e-5
    float utci =  Ta
        + 0.607562052f
        - 0.0227712343f  * Ta
        + 8.06470461e-4f * Ta*Ta
        - 1.54271372e-4f * Ta*Ta*Ta
        - 3.24651735e-6f * Ta*Ta*Ta*Ta
        + 7.32602852e-8f * Ta*Ta*Ta*Ta*Ta
        + 1.35959073e-9f * Ta*Ta*Ta*Ta*Ta*Ta
        - 2.25836520f    * Va
        + 0.0880326035f  * Ta*Va
        + 0.00216844454f * Ta*Ta*Va
        - 1.53347087e-5f * Ta*Ta*Ta*Va
        - 5.72983704e-7f * Ta*Ta*Ta*Ta*Va
        - 2.55090145e-9f * Ta*Ta*Ta*Ta*Ta*Va
        - 0.751269505f   * Va*Va
        - 0.00408350271f * Ta*Va*Va
        - 5.21670675e-5f * Ta*Ta*Va*Va
        + 1.94544667e-6f * Ta*Ta*Ta*Va*Va
        + 1.14099092e-8f * Ta*Ta*Ta*Ta*Va*Va
        + 0.158137256f   * Va*Va*Va
        - 6.57263143e-4f * Ta*Va*Va*Va
        + 2.22697524e-7f * Ta*Ta*Va*Va*Va
        - 4.16117031e-8f * Ta*Ta*Ta*Va*Va*Va
        - 0.0127762753f  * Va*Va*Va*Va
        + 9.66891875e-6f * Ta*Va*Va*Va*Va
        + 2.52785852e-9f * Ta*Ta*Va*Va*Va*Va
        + 4.56306672e-4f * Va*Va*Va*Va*Va
        - 1.74202546e-7f * Ta*Va*Va*Va*Va*Va
        - 5.91491269e-6f * Va*Va*Va*Va*Va*Va
        + 0.398374029f   * D
        + 1.83945314e-4f * Ta*D
        - 1.73754510e-4f * Ta*Ta*D
        - 7.60781159e-7f * Ta*Ta*Ta*D
        + 3.77830287e-8f * Ta*Ta*Ta*Ta*D
        + 4.43354067e-9f * Ta*Ta*Ta*Ta*Ta*D
        - 0.0514190526f  * Va*D
        + 2.61913559e-4f * Ta*Va*D
        - 6.77223951e-6f * Ta*Ta*Va*D
        + 2.45267816e-8f  * Va*Va*D
        + 1.87674476e-3f * RH;  // simplified humidity term

    return utci;
}

String calcUTCICategory(float utci) {
    if (utci >  46.0f) return "Extreme Heat Stress";
    if (utci >  38.0f) return "Very Strong Heat";
    if (utci >  32.0f) return "Strong Heat";
    if (utci >  26.0f) return "Moderate Heat";
    if (utci >   9.0f) return "No Thermal Stress";
    if (utci >   0.0f) return "Slight Cold";
    if (utci > -13.0f) return "Moderate Cold";
    if (utci > -27.0f) return "Strong Cold";
    return "Extreme Cold";
}

// ─────────────────────────────────────────────────────────────────────────────
// MEDICAL: Required Air Changes per Hour (ACH) for CO2 dilution
// Based on ASHRAE 62.1 mass-balance equation:
//   ACH = Q/V  where Q = (G * 3600) / ((Ci - Co) * V)
//   G = CO2 generation rate [L/h per person], estimated from occupant load
//   Ci = indoor CO2 [ppm],  Co = 420 ppm (outdoor baseline, 2026 calibrated)
// Output: minimum ACH to bring CO2 below 1000 ppm (ASHRAE guideline)
// ─────────────────────────────────────────────────────────────────────────────
float calcRequiredACH(float co2_ppm) {
    const float Co = 420.0f;  // outdoor CO2 baseline (2026 actual)
    const float Ci_target = 1000.0f;
    const float G_Lph = 18.0f; // CO2 generation per person ~18 L/h (moderate activity)
    if (co2_ppm <= Co) return 0.0f;
    // Assuming a 20 m² × 2.7 m room (typ. monitoring room volume = 54 m³)
    // ACH ∝ ΔCO2 excess above target
    float excess = co2_ppm - Ci_target;
    if (excess <= 0) return 0.0f;
    // Rough ACH: proportional to how much excess must be diluted
    return min(20.0f, 0.02f * excess);
}

// ─────────────────────────────────────────────────────────────────────────────
// MEDICAL: Radon accumulation risk
// Radon (222Rn) is a decay product of soil radium, driven into buildings by:
//   1. Low atmospheric pressure (negative stack effect pulls soil gas in)
//   2. Calm wind (no ventilation pressure differential)
//   3. High soil moisture (rain seals pathways)
// Formula is semi-empirical, scaled 0–10.
// High-risk threshold: > 300 Bq/m³ (EU reference level) — we provide proxy only
// ─────────────────────────────────────────────────────────────────────────────
float calcRadonRisk(float slp_delta_3h, float wind_kph, float rain_mm) {
    float risk = 5.0f; // baseline
    // Pressure drop → radon suction
    if (slp_delta_3h < -2.0f)  risk += min(3.0f, fabsf(slp_delta_3h) * 0.5f);
    // Calm wind → no ventilation
    if (wind_kph < 3.0f)       risk += 1.5f;
    else if (wind_kph > 15.0f) risk -= 2.0f;
    // Recent rain → sealed ground → accumulates indoors
    if (rain_mm > 5.0f)        risk += 1.0f;
    // Pressure rise → pushes radon back into soil
    if (slp_delta_3h > 3.0f)   risk -= 1.5f;
    return max(0.0f, min(10.0f, risk));
}

// ─────────────────────────────────────────────────────────────────────────────
// GEOPHYSICS: Seismic proxy from ILPS22QS QVAR
// The ILPS22QS has a QVAR (quasi-electrostatic) input that is sensitive to
// electrostatic field changes.  These correlate (weakly) with:
//   • Ionospheric coupling before M>5 earthquakes (Freund model)
//   • Thunderstorm electrical field
//   • Ground charge accumulation during clear dry weather
// We track the time-derivative (rate of change) of the QVAR value as proxy.
// Output: 0–10 scale, 0 = quiet, >6 = notable electrical disturbance detected
// ─────────────────────────────────────────────────────────────────────────────
static float qvar_prev = -9999.0f;
static unsigned long qvar_prev_ts = 0;

// =============================================================================
// SPACE WEATHER — BLOCK 2
// =============================================================================

// ─────────────────────────────────────────────────────────────────────────────
// dB/dt — Sudden Storm Commencement / CME Shock Wave Detector
//
// Formula (from image): √(ΔBx² + ΔBy² + ΔBz²) / Δt   [nT/s]
//
// SSC is the sharp, rapid increase in |B| when a solar CME (coronal mass
// ejection) shockwave hits Earth's magnetosphere.  A spike > 10 nT/min
// is considered a notable geomagnetic disturbance onset (IAGA definition).
//
// Sources: Araki (1994) Geophys. Mono.; IAGA SSC reporting threshold
// dB/dt > 1 nT/s = moderate SSC; > 5 nT/s = major CME impact
//
// Implementation: store previous X/Y/Z reading and elapsed time.
// Returns nT/s; also returns SSC classification string.
// ─────────────────────────────────────────────────────────────────────────────
static float bmm_prev_x = 0, bmm_prev_y = 0, bmm_prev_z = 0;
static unsigned long bmm_prev_ts = 0;

float calcDBdt_nTs(float bx, float by, float bz) {
    unsigned long now_ms = millis();
    float dbdt = 0.0f;
    if (bmm_prev_ts > 0) {
        float dt_s = (now_ms - bmm_prev_ts) / 1000.0f;
        if (dt_s > 0.5f) {
            float dbx = (bx - bmm_prev_x) * 1000.0f;  // µT → nT
            float dby = (by - bmm_prev_y) * 1000.0f;
            float dbz = (bz - bmm_prev_z) * 1000.0f;
            dbdt = sqrtf(dbx*dbx + dby*dby + dbz*dbz) / dt_s;
        }
    }
    bmm_prev_x = bx; bmm_prev_y = by; bmm_prev_z = bz;
    bmm_prev_ts = now_ms;
    return dbdt;
}

String classifySSC(float dbdt_nTs) {
    if (dbdt_nTs < 0.5f)  return "Quiet";
    if (dbdt_nTs < 2.0f)  return "Low Activity";
    if (dbdt_nTs < 5.0f)  return "Moderate SSC";
    if (dbdt_nTs < 10.0f) return "Strong SSC";
    return "CME Impact";
}

// ─────────────────────────────────────────────────────────────────────────────
// Forbush Decrease Detector
//
// Formula (from image): ΔI = [(I_corr(t) - I_7day_avg) / I_7day_avg] × 100%
//
// A Forbush decrease is a rapid drop (3–10%) of galactic cosmic ray flux
// caused by magnetic shielding from a solar CME.  It typically precedes
// geomagnetic storms by 1-3 days after a solar eruption.
//
// Typical signature: GCR drops 3-10% over 6-24 hours, recovers over days.
// Threshold: -3% = minor, -6% = significant, > -10% = major Forbush event.
//
// References: Lockwood (1971); Cane (2000) Space Sci. Rev.
// ─────────────────────────────────────────────────────────────────────────────
#define GCR_HIST_SIZE 288  // 288 × 30-min = 6 days rolling average (≈ 7-day)
static float gcr_hist[GCR_HIST_SIZE] = {0};
static int   gcr_hist_idx = 0;
static int   gcr_hist_cnt = 0;

void updateGCRHistory(float gcr_corrected) {
    if (gcr_corrected <= 0) return;
    gcr_hist[gcr_hist_idx] = gcr_corrected;
    gcr_hist_idx = (gcr_hist_idx + 1) % GCR_HIST_SIZE;
    if (gcr_hist_cnt < GCR_HIST_SIZE) gcr_hist_cnt++;
}

float calcForbushDecrease(float gcr_now) {
    if (gcr_hist_cnt < 24) return 0.0f;  // need at least 12h of data
    float sum = 0; int cnt = 0;
    for (int i = 0; i < gcr_hist_cnt; i++) {
        if (gcr_hist[i] > 0) { sum += gcr_hist[i]; cnt++; }
    }
    if (cnt == 0) return 0.0f;
    float avg = sum / cnt;
    return ((gcr_now - avg) / avg) * 100.0f;  // % deviation from baseline
}

String classifyForbush(float forbush_pct) {
    if (forbush_pct > -2.0f)   return "Normal";
    if (forbush_pct > -5.0f)   return "Minor Decrease";
    if (forbush_pct > -8.0f)   return "Significant Forbush";
    return "Major Forbush Event";
}

float calcSeismicProxy(float qvar_now) {
    float rate = 0.0f;
    unsigned long now_ms = millis();
    if (qvar_prev > -9999.0f && (now_ms - qvar_prev_ts) > 0) {
        float dt_min = (now_ms - qvar_prev_ts) / 60000.0f;
        if (dt_min > 0.0f) rate = fabsf(qvar_now - qvar_prev) / dt_min;
    }
    qvar_prev    = qvar_now;
    qvar_prev_ts = now_ms;
    // Map rate to 0–10 scale; ~1000 units/min = moderate disturbance
    float proxy = min(10.0f, rate / 200.0f);
    return proxy;
}

// ─────────────────────────────────────────────────────────────────────────────
// ASTRO/SPACE: Atmospheric stability (convective instability indicator)
// Uses Equivalent Potential Temperature lapse estimate:
//   • theta_e rising with altitude → stable (no convection)
//   • theta_e roughly constant → neutral
//   • theta_e falling → UNSTABLE → thunderstorm potential
// We proxy stability from: enclosure temp (surface) vs outdoor temp (ambient)
// Combined with LCL altitude to give a simple instability index 0–10.
// ─────────────────────────────────────────────────────────────────────────────
float calcConvectiveInstability(float theta_e_surface, float lcl_m, float slp_delta) {
    float idx = 0.0f;
    // High θe + low LCL → conditionally unstable
    if (theta_e_surface > 345.0f) idx += 3.0f;
    else if (theta_e_surface > 335.0f) idx += 1.5f;
    if (lcl_m < 500.0f)  idx += 3.0f;
    else if (lcl_m < 1000.0f) idx += 1.5f;
    // Rapid pressure fall is a classic precursor
    if (slp_delta < -3.0f)  idx += 2.0f;
    else if (slp_delta < -1.0f) idx += 1.0f;
    return min(10.0f, idx);
}

// ─────────────────────────────────────────────────────────────────────────────
// Station constants used across multiple formulas
// Nowy Sącz, PL:  geographic lat 49.62°N
// Geomagnetic lat ≈ geographic + 11.5° → ~61.1°N effective
// ─────────────────────────────────────────────────────────────────────────────
static const float STATION_GEO_LAT   = 49.62f;
static const float STATION_GEOMAG_LAT = 61.1f; // for aurora calculations

// --- ANOMALY ENGINE EVALUATION ---
String detectSingleAnomaly(float current, float* hist, float threshold) {
    if (current <= -900.0f) return "OFF"; // Ignore missing data
    
    float sum = 0;
    int count = 0;
    int total = anom_hist_filled ? ANOMALY_HIST_SIZE : anom_hist_idx;
    
    for(int i = 0; i < total; i++) {
        if (hist[i] > -900.0f) {
            sum += hist[i];
            count++;
        }
    }
    
    if (count >= 3) { // Require at least 3 historical points to form a baseline
        float avg = sum / count;
        if (abs(current - avg) > threshold) {
            ATLAS_LOG("   [ANOMALY ENGINE] Shift vs Baseline! Val: %.1f, Avg: %.1f, Thr: %.1f\n", current, avg, threshold);
            return "ON";
        }
    }
    return "OFF";
}

void updateAnomalyBuffers(float t, float h, float pm, float voc, float co2, float p, float cpm) {
    anom_hist_temp[anom_hist_idx] = t; anom_hist_hum[anom_hist_idx] = h;
    anom_hist_pm25[anom_hist_idx] = pm; anom_hist_voc[anom_hist_idx] = voc;
    anom_hist_co2[anom_hist_idx] = co2;
    anom_hist_press[anom_hist_idx] = p;
    anom_hist_cpm[anom_hist_idx] = cpm;
    
    anom_hist_idx++;
    if (anom_hist_idx >= ANOMALY_HIST_SIZE) { anom_hist_idx = 0; anom_hist_filled = true; }
}

// o()xxxx[{::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::>
// 4. SENSOR LIFECYCLES, MODULAR INIT
// o()xxxx[{::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::>

void bme688Callback(const bme68xData data, const bsecOutputs outputs, Bsec2 bsec) {
    if (!outputs.nOutputs) return;
    for (uint8_t i=0; i<outputs.nOutputs; i++) {
        switch (outputs.output[i].sensor_id) {
            case BSEC_OUTPUT_RAW_TEMPERATURE: payload["sensors"]["BME688_Temp_Raw"] = outputs.output[i].signal; break;
            case BSEC_OUTPUT_RAW_HUMIDITY: payload["sensors"]["BME688_Hum_Raw"] = outputs.output[i].signal; break;
            case BSEC_OUTPUT_RAW_PRESSURE: payload["sensors"]["BME688_Press_Raw"] = outputs.output[i].signal; break;
            case BSEC_OUTPUT_RAW_GAS: payload["sensors"]["BME688_Gas_Res_Raw"] = outputs.output[i].signal; break;
            case BSEC_OUTPUT_IAQ: payload["sensors"]["BME688_IAQ"] = outputs.output[i].signal; payload["sensors"]["BME688_IAQ_Accuracy"] = outputs.output[i].accuracy; break;
            case BSEC_OUTPUT_STATIC_IAQ: payload["sensors"]["BME688_Static_IAQ"] = outputs.output[i].signal; break;
            case BSEC_OUTPUT_CO2_EQUIVALENT: payload["sensors"]["BME688_eCO2"] = outputs.output[i].signal; break;
            case BSEC_OUTPUT_BREATH_VOC_EQUIVALENT: payload["sensors"]["BME688_bVOC"] = outputs.output[i].signal; break;
            case BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE: payload["sensors"]["BME688_Comp_Temp"] = outputs.output[i].signal; break;
            case BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY: payload["sensors"]["BME688_Comp_Hum"] = outputs.output[i].signal; break;
            case BSEC_OUTPUT_GAS_PERCENTAGE: payload["sensors"]["BME688_Gas_Percentage"] = outputs.output[i].signal; break;
            case BSEC_OUTPUT_COMPENSATED_GAS: payload["sensors"]["BME688_Comp_Gas"] = outputs.output[i].signal; break;
            case BSEC_OUTPUT_STABILIZATION_STATUS: payload["sensors"]["BME688_Stab_Status"] = outputs.output[i].signal; break;
            case BSEC_OUTPUT_RUN_IN_STATUS: payload["sensors"]["BME688_RunIn_Status"] = outputs.output[i].signal; break;
            case BSEC_OUTPUT_GAS_ESTIMATE_1: payload["sensors"]["BME688_Gas_Est_1"] = outputs.output[i].signal; break;
            case BSEC_OUTPUT_GAS_ESTIMATE_2: payload["sensors"]["BME688_Gas_Est_2"] = outputs.output[i].signal; break;
            case BSEC_OUTPUT_GAS_ESTIMATE_3: payload["sensors"]["BME688_Gas_Est_3"] = outputs.output[i].signal; break;
            case BSEC_OUTPUT_GAS_ESTIMATE_4: payload["sensors"]["BME688_Gas_Est_4"] = outputs.output[i].signal; break;
            case BSEC_OUTPUT_RAW_GAS_INDEX: payload["sensors"]["BME688_Raw_Gas_Index"] = outputs.output[i].signal; break;
        }
    }
}

// --- WDROŻENIE SPRZĘTOWEGO PCNT DLA GEIGERA ---
bool initGeiger() {
    pinMode(GEIGER_IRQ_PIN, INPUT_PULLUP);
    
    pcnt_config_t pcnt_config = {};
    pcnt_config.pulse_gpio_num = GEIGER_IRQ_PIN;
    pcnt_config.ctrl_gpio_num = PCNT_PIN_NOT_USED;
    pcnt_config.lctrl_mode = PCNT_MODE_KEEP;
    pcnt_config.hctrl_mode = PCNT_MODE_KEEP;
    pcnt_config.pos_mode = PCNT_COUNT_DIS;     // Reaguj tylko na zbocze opadające
    pcnt_config.neg_mode = PCNT_COUNT_INC;     
    pcnt_config.counter_h_lim = 30000;
    pcnt_config.counter_l_lim = -1;
    pcnt_config.unit = PCNT_UNIT_0;
    pcnt_config.channel = PCNT_CHANNEL_0;
    
    pcnt_unit_config(&pcnt_config);
	// [BUG_HW#0001]
    // Filtr 1023 chroni przed szumami (ok. 12.5 us przy 80MHz APB)
	// UWAGA, PROBLEMY Z GND, SPRAWDZIC HARDWARE
	// ZMIENIC SYSTEM ZASILANIA NA DFROBOT dfr0535
	// ZASTANOWIC SIE NAD IMPLEMENTACJA FILTRU WYCINAJACEGO NIECHCIANE 
	// PEAKI JAKO WORKAROUND SOFTWARE'owy, ale ostroznie
	// TO TYLKO MASKOWANIE PROBLEMU NIE JEGO ROZWIAZANIE
    pcnt_set_filter_value(PCNT_UNIT_0, 1023); 
    pcnt_filter_enable(PCNT_UNIT_0);
    
    pcnt_counter_pause(PCNT_UNIT_0);
    pcnt_counter_clear(PCNT_UNIT_0);
    pcnt_counter_resume(PCNT_UNIT_0);

    geiger_pulses = 0; 
    ATLAS_LOG("[HW INIT] CH[-]: SEN0463 Geiger Counter -> ONLINE (PCNT HW Active)\n");
    geiger_ok = true;
    return true;
}

bool initBME280() {
    if(tcaselect(MUX_BME280, CH_BME280)) {
        if(bme280.begin(0x76) || bme280.begin(0x77)) { 
            bme280.setSampling(Adafruit_BME280::MODE_NORMAL);
            ATLAS_LOG("[HW INIT] CH0: BME280 BMS Monitor -> ONLINE\n"); 
            bme280_ok = true; return true;
        }
    }
    bme280_ok = false; return false;
}

bool initBMV080() {
    if(!tcaselect(MUX_BMV080, CH_BMV080)) return false;

    if (bmv080.begin(0x57, Wire)) { 
        ATLAS_LOG("[HW INIT] CH1: BMV080 -> I2C connected. Applying config...\n");
        bmv080.init();
        
        bmv080.setMode(0); 
        delay(50);
        
        prefs.begin("bmv_cfg", true);
        uint16_t integrationTime = prefs.getUShort("intTime", 10); 

        bool doVibFilter = prefs.getBool("vibFilter", true); 
        bool doObsDetect = prefs.getBool("obsDetect", false);
        uint8_t algo = prefs.getUChar("algo", 2); 
        prefs.end();

        bmv080.setMeasurementAlgorithm(algo);
		// do przeanalizowania na środowisku docelowym, czy korzystac
		// z wbudowanego w BMV080 filtru przeciw wibracyjnemu
		// odniesienie do SDK BOSCH
        bmv080.setDoVibrationFiltering(doVibFilter);
        bmv080.setDoObstructionDetection(doObsDetect);
        if (integrationTime > 0.0f) bmv080.setIntegrationTime(integrationTime);
        
        bmv080.setMode(1);
        ATLAS_LOG("[HW INIT] CH1: BMV080 (PM Laser) -> FORCED CONTINUOUS LASER ACTIVATED\n");
        
        bmv080_ok = true; 
        return true;
    }
    
    ATLAS_LOG("[HW INIT] CH1: BMV080 -> FAILED TO CONNECT!\n");
    bmv080_ok = false;
    return false;
}

bool initINA219() {
    if(tcaselect(MUX_INA219, CH_INA219)) {
        if(ina219.begin()) { 
            ATLAS_LOG("[HW INIT] CH2: INA219 Solar Array -> ONLINE\n");
            ina219_ok = true; return true;
        }
    }
    ina219_ok = false; return false;
}

bool initMS8607() {
    if(tcaselect(MUX_MS8607, CH_MS8607)) {
        if(ms8607.begin()) { 
            ATLAS_LOG("[HW INIT] CH3: MS8607 Precision PHT -> ONLINE\n");
            ms8607_ok = true; return true;
        }
    }
    ms8607_ok = false; return false;
}

bool initMAX17048() {
    if(tcaselect(MUX_MAX17048, CH_MAX17048)) {
        if(bat.begin()) { 
            bat.sleep(false);
            bat.wake();
            ATLAS_LOG("[HW INIT] CH3: MAX17048 Fuel Gauge -> WAKING UP ADC\n");
            max17048_ok = true; return true;
        }
    }
    max17048_ok = false; return false;
}

bool initBME688() {
    if(!tcaselect(MUX_BME688, CH_BME688)) return false;
    
    bsecSensor sList[] = { 
        BSEC_OUTPUT_RAW_TEMPERATURE, BSEC_OUTPUT_RAW_HUMIDITY, BSEC_OUTPUT_RAW_PRESSURE, 
        BSEC_OUTPUT_RAW_GAS, BSEC_OUTPUT_IAQ, BSEC_OUTPUT_STATIC_IAQ, BSEC_OUTPUT_GAS_PERCENTAGE,
        BSEC_OUTPUT_CO2_EQUIVALENT, BSEC_OUTPUT_BREATH_VOC_EQUIVALENT, BSEC_OUTPUT_COMPENSATED_GAS,
        BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE, BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY,
        BSEC_OUTPUT_STABILIZATION_STATUS, BSEC_OUTPUT_RUN_IN_STATUS,
        BSEC_OUTPUT_GAS_ESTIMATE_1, BSEC_OUTPUT_GAS_ESTIMATE_2, BSEC_OUTPUT_GAS_ESTIMATE_3, BSEC_OUTPUT_GAS_ESTIMATE_4,
        BSEC_OUTPUT_RAW_GAS_INDEX
    };
    if (envSensor.begin(BME68X_I2C_ADDR_HIGH, Wire)) {
        if (bme688Config.customAiProfile != nullptr) {
            envSensor.setConfig(bme688Config.customAiProfile);
        }

		envSensor.setTemperatureOffset(bme688Config.tempOffset);
        
        // REDUNDANT MEMORY LOAD (EEPROM -> NVS Fallback)
        uint8_t state[BSEC_MAX_STATE_BLOB_SIZE];
        bool stateLoaded = false;
        
        // Próba 1: Zewnętrzny EEPROM (Adres 0x0010, żeby ominąć sektor zerowy)
        if (i2c_mem_ok && readExtEEPROM(0x0010, state, BSEC_MAX_STATE_BLOB_SIZE)) {
            // Walidacja czy to nie są czyste FF
            if (state[0] != 0xFF && state[1] != 0xFF) {
                envSensor.setState(state);
                ATLAS_LOG("[HW INIT] CH4: BME690 AI Brain -> Restored BSEC State from EXTERNAL EEPROM.\n");
                stateLoaded = true;
            }
        }
        
        // Próba 2: Spadochron (NVS Flash), jeśli EEPROM zawiódł lub jest pusty
        if (!stateLoaded) {
            prefs.begin("bsec_data", true);
            if (prefs.getBytesLength("bsec_state") == BSEC_MAX_STATE_BLOB_SIZE) {
                prefs.getBytes("bsec_state", state, BSEC_MAX_STATE_BLOB_SIZE); 
                envSensor.setState(state);
                ATLAS_LOG("[HW INIT] CH4: BME690 AI Brain -> Restored BSEC State from NVS (Fallback).\n");
            } else {
                ATLAS_LOG("[HW INIT] CH4: BME690 AI Brain -> No state found. Cold Start (Learning Mode).\n");
            }
            prefs.end();
        }
        
        envSensor.updateSubscription(sList, 19, bme688Config.sampleRate);
        checkBsecStatus(envSensor);
        if (envSensor.status < BSEC_OK) {
            ATLAS_LOG("[HW INIT] CH4: BME690 -> CRITICAL FAILURE. Wiping NVS...\n");
            prefs.begin("bsec_data", false); prefs.clear(); prefs.end();
            bme688_ok = false; return false;
        } 
        
        envSensor.attachCallback(bme688Callback);
        ATLAS_LOG("[HW INIT] CH4: BME690 (IAQ Gas Brain) -> ONLINE & HEATING MOX LAYER\n");
        bme688_ok = true; return true;
    }
    
    ATLAS_LOG("[HW INIT] CH4: BME690 -> FAILED TO CONNECT!\n");
    bme688_ok = false;
    return false;
}

// --- INIT NEW SENSORS CH3 ---
bool initSHT45() {
    if(tcaselect(MUX_SHT45, CH_SHT45)) {
        if(sht45.begin()) {
            sht45.setPrecision(SHT4X_HIGH_PRECISION);
            sht45.setHeater(SHT4X_NO_HEATER);
            ATLAS_LOG("[HW INIT] CH3: SHT45 Precision Temp/Hum -> ONLINE\n");
            sht45_ok = true; return true;
        }
    }
    sht45_ok = false; return false;
}

bool initSGP41() {
    if(tcaselect(MUX_SGP41, CH_SGP41)) {
        sgp41.begin(Wire);
        uint16_t serialNumber[3];
        if (sgp41.getSerialNumber(serialNumber) == 0) {
            ATLAS_LOG("[HW INIT] CH3: SGP41 VOC/NOx -> ONLINE & WARMING UP\n");
            sgp41_ok = true; 
            return true;
        }
    }
    sgp41_ok = false; return false;
}

bool initTSL2591() {
    if(tcaselect(MUX_TSL2591, CH_TSL2591)) {
        if(tsl2591.begin()) {
            tsl2591.setGain(TSL2591_GAIN_LOW);
            tsl2591.setTiming(TSL2591_INTEGRATIONTIME_100MS);
            ATLAS_LOG("[HW INIT] CH3: TSL2591 HDR Light -> ONLINE (OUTDOOR MODE)\n");
            tsl2591_ok = true; return true;
        }
    }
    tsl2591_ok = false; return false;
}

bool initOPT4048() {
    if(tcaselect(MUX_OPT4048, CH_OPT4048)) {
        if(opt4048.begin(0x44)) { // 0x44 to typowy adres I2C układu OPT4048
            ATLAS_LOG("[HW INIT] CH6: OPT4048 High-Speed Color -> ONLINE\n");
            opt4048_ok = true; return true;
        }
    }
    opt4048_ok = false; return false;
}

bool initTCS34725() {
    if(tcaselect(MUX_TCS34725, CH_TCS34725)) {
        if(tcs34725.begin()) {
            ATLAS_LOG("[HW INIT] CH6: TCS34725 RGB Spectrum -> ONLINE\n");
            tcs34725_ok = true; return true;
        }
    }
    tcs34725_ok = false; return false;
}

bool initI2CMemory() {
    // Parallel on main bus - no tcaselect needed
    Wire.beginTransmission(0x50);
    if(Wire.endTransmission() == 0) {
        ATLAS_LOG("[HW INIT] BUS: I2C Memory (0x50) -> ONLINE (AI Baseline Storage)\n");
        i2c_mem_ok = true; return true;
    }
    i2c_mem_ok = false; return false;
}

bool initAS7343() {
    if(tcaselect(MUX_AS7343, CH_AS7343)) {
        // AS7343/AS7341 share address 0x39. Read chip ID first.
        // Register 0x92:
        //   0x81 -> AS7343 (expected)
        //   0x09 -> AS7341 (older variant)
        uint8_t chip_id = 0x00;
        bool id_read_ok = false;
        Wire.beginTransmission(0x39);
        Wire.write(0x92);
        if (Wire.endTransmission(false) == 0 && Wire.requestFrom((uint8_t)0x39, (uint8_t)1) == 1) {
            chip_id = Wire.read();
            id_read_ok = true;
        }

        if (id_read_ok) {
            if (chip_id == 0x81) {
                ATLAS_LOG("[HW INIT] CH%d: AS7343 Chip ID 0x81 confirmed.\n", CH_AS7343);
            } else if (chip_id == 0x09) {
                ATLAS_LOG("[HW INIT][WARN] CH%d: AS7341 detected (ID 0x09). Running in compatibility mode.\n", CH_AS7343);
                payload["alerts"]["AS7343_ID_Mismatch"] = "AS7341 detected";
            } else {
                ATLAS_LOG("[HW INIT][WARN] CH%d: Unknown spectral chip ID 0x%02X at 0x39.\n", CH_AS7343, chip_id);
                payload["alerts"]["AS7343_ID_Mismatch"] = "Unknown ID";
            }
            payload["sensors"]["AS7343_Chip_ID"] = chip_id;
        } else {
            ATLAS_LOG("[HW INIT][WARN] CH%d: Could not read spectral chip ID register 0x92.\n", CH_AS7343);
            payload["alerts"]["AS7343_ID_Mismatch"] = "ID read failed";
        }

        if(as7343.begin()) {
            as7343.powerOn();
            as7343.setAutoSmux(AUTOSMUX_18_CHANNELS);
            as7343.setAgain(as_gain_values[optics_gain_idx]);
            as7343.enableSpectralMeasurement();

            ATLAS_LOG("[HW INIT] CH%d: AS7343/AS7341 Spectrometer @0x39 -> ONLINE | Gain: %.1fx\n", CH_AS7343, as_gain_values[optics_gain_idx]);
            as7343_ok = true;
            return true;
        }
    }
    as7343_ok = false; return false;
}

bool initVEML7700() {
    if(tcaselect(MUX_VEML, CH_VEML)) {
        // VEML7700 address is 0x10 (default)
        if(veml.begin()) { 
            veml.powerSaveEnable(false);
            ATLAS_LOG("[HW INIT] CH%d: VEML7700 Lux (0x10) -> POWER ENABLED\n", CH_VEML);
            veml_ok = true; return true;
        }
    }
    veml_ok = false; return false;
}

bool initLTR390() {
    if(tcaselect(MUX_LTR, CH_LTR)) {
        if(ltr.begin()) { 
            ltr.setUVSMode();
            ATLAS_LOG("[HW INIT] CH4: LTR390 UV -> ONLINE\n");
            ltr_ok = true; return true;
        }
    }
    ltr_ok = false; return false;
}

bool initAS3935() {
    if(tcaselect(MUX_AS3935, CH_AS3935)) {
        pinMode(AS3935_IRQ_PIN, INPUT_PULLUP);
        if(lightning.begin() == 0) { 
            lightning.setOutdoors();
            attachInterrupt(digitalPinToInterrupt(AS3935_IRQ_PIN), detectLightning, RISING);
            ATLAS_LOG("[HW INIT] CH5: AS3935 Lightning -> LISTENING FOR RF SPIKES\n");
            as3935_ok = true; return true;
        }
    }
    as3935_ok = false; return false;
}

bool initSCD41() {
    if(tcaselect(MUX_SCD41, CH_SCD41)) {
        scd41.begin(Wire, 0x62); scd41.stopPeriodicMeasurement();
        uint16_t scd41_status = 0;
        if (scd41.performSelfTest(scd41_status) == 0 && scd41_status == 0) { 
            ATLAS_LOG("[HW INIT] CH7: SCD41 (Absolute NDIR) -> ONLINE (Single-Shot Mode)\n"); 
            scd41_ok = true; return true; 
        }
    }
    scd41_ok = false; return false;
}

bool initBMP585() {
    if(tcaselect(MUX_BMP585, CH_BMP585) && bmp585.begin()) {
        bmp585.setTemperatureOversampling((bmp5xx_oversampling_t)BMP5_OVERSAMPLING_8X); 
        bmp585.setPressureOversampling((bmp5xx_oversampling_t)BMP5_OVERSAMPLING_128X);
        bmp585.setIIRFilterCoeff((bmp5xx_iir_filter_t)BMP5_IIR_FILTER_COEFF_31);
        ATLAS_LOG("[HW INIT] CH7: BMP585 (Zambretti Core) -> ONLINE\n"); 
        bmp585_ok = true; return true;
    }
    bmp585_ok = false; return false;
}

bool initILPS22QS() { 
    if(tcaselect(MUX_ILPS, CH_ILPS)) {
        ilps.begin(); 
        if(ilps.Enable() == 0) { 
            
            // --- INŻYNIERYJNY BYPASS QVAR (Ręczna konfiguracja przez I2C) ---
            Wire.beginTransmission(0x5C); // Domyślny adres I2C dla ILPS22QS
            Wire.write(0x12);             // Rejestr CTRL3 (Konfiguracja sprzętowa)
            if (Wire.endTransmission(false) == 0) {
                Wire.requestFrom((uint16_t)0x5C, (uint8_t)1);
                if (Wire.available() == 2) {
                    uint8_t ctrl3 = Wire.read();
                    Wire.beginTransmission(0x5C);
                    Wire.write(0x12);
                    Wire.write(ctrl3 | 0x80); // Ustawienie bitu 7 (AH_QVAR_EN) na wartość 1 (WŁĄCZ)
                    Wire.endTransmission();
                }
            }
            // ---------------------------------------------------------------
            
            ATLAS_LOG("[HW INIT] CH7: ILPS22QST (with QVAR Bypass) -> ONLINE\n");
            ilps_ok = true; 
            return true; 
        }
    }
    ilps_ok = false; return false; 
}

bool initAS7331() { 
    if(tcaselect(MUX_AS7331, CH_AS7331)) {
        if(as7331.begin(0x74)) { 
            as7331.prepareMeasurement(MEAS_MODE_CONT); // Tryb ciągły
            as7331.setStartState(true);                // Start pomiarów w tle
            ATLAS_LOG("[HW INIT] CH%d: AS7331 (Medical UV 0x74) -> ONLINE\n", CH_AS7331); 
            as7331_ok = true; 
            return true; 
        } 
    }
    as7331_ok = false; return false; 
}

bool initBMM350() {
    if(tcaselect(MUX_BMM350, CH_BMM350)) {
        if(bmm350.begin()) {
            bmm350.setOperationMode(eBmm350NormalMode);
            bmm350.setPresetMode(BMM350_PRESETMODE_ENHANCED, BMM350_DATA_RATE_25HZ);
            ATLAS_LOG("[HW INIT] 0x70/CH%d: BMM350 Precision Magnetometer -> ONLINE\n", CH_BMM350); 
            bmm350_ok = true; return true; 
        } 
    }
    ATLAS_LOG("[HW INIT] 0x70/CH%d: BMM350 -> FAILED\n", CH_BMM350);
    bmm350_ok = false; return false; 
}

// -----------------------------------------------------------------------
// LSM6DSOX — 6-axis IMU: 3-axis accelerometer + 3-axis gyroscope
// Mounted on MUX 0x70 CH1 together with LIS3MDL (different I2C address)
// Default I2C address: 0x6A (SDO low)
// -----------------------------------------------------------------------
bool initLSM6DSOX() {
    if(tcaselect(MUX_LSM6DSOX, CH_LSM6DSOX)) {
        if(lsm6dsox.begin_I2C(0x6A)) {
            // High-performance mode: ODR 104 Hz, full scale ±4g / ±500 dps
            lsm6dsox.setAccelRange(LSM6DS_ACCEL_RANGE_4_G);
            lsm6dsox.setAccelDataRate(LSM6DS_RATE_104_HZ);
            lsm6dsox.setGyroRange(LSM6DS_GYRO_RANGE_500_DPS);
            lsm6dsox.setGyroDataRate(LSM6DS_RATE_104_HZ);
            ATLAS_LOG("[HW INIT] 0x70/CH%d: LSM6DSOX IMU (0x6A) -> ONLINE | Accel:±4g Gyro:±500dps @104Hz\n", CH_LSM6DSOX);
            lsm6dsox_ok = true; return true;
        }
    }
    ATLAS_LOG("[HW INIT] 0x70/CH%d: LSM6DSOX -> FAILED\n", CH_LSM6DSOX);
    lsm6dsox_ok = false; return false;
}

// -----------------------------------------------------------------------
// LIS3MDL — 3-axis magnetometer (companion to LSM6DSOX, same MUX channel)
// Default I2C address: 0x1E (SDO/SA1 low)
// Range: ±4/±8/±12/±16 gauss, ODR up to 1000 Hz
// -----------------------------------------------------------------------
bool initLIS3MDL() {
    if(tcaselect(MUX_LIS3MDL, CH_LIS3MDL)) {
        if(lis3mdl.begin_I2C(0x1E)) {
            lis3mdl.setPerformanceMode(LIS3MDL_MEDIUMMODE);
            lis3mdl.setOperationMode(LIS3MDL_CONTINUOUSMODE);
            lis3mdl.setDataRate(LIS3MDL_DATARATE_155_HZ);
            lis3mdl.setRange(LIS3MDL_RANGE_4_GAUSS);
            ATLAS_LOG("[HW INIT] 0x70/CH%d: LIS3MDL Magnetometer (0x1E) -> ONLINE | ±4G @155Hz\n", CH_LIS3MDL);
            lis3mdl_ok = true; return true;
        }
    }
    ATLAS_LOG("[HW INIT] 0x70/CH%d: LIS3MDL -> FAILED\n", CH_LIS3MDL);
    lis3mdl_ok = false; return false;
}

// ─────────────────────────────────────────────────────────────────────────────
// MLX90640 Thermal Camera — sky cloud cover detection
// The sky-facing IR matrix scans a ~55°×35° FOV.
// "Cosmic Cold Sink": clear sky is ~230 K (-40 to -60°C window in atmosphere)
// Clouds (any altitude): warmer than -10°C → appear warm in the IR image
// Method: count pixels < -10°C (sky window) as "clear sky pixels"
// Cloud cover % = (warm pixels / total pixels) × 100
// ─────────────────────────────────────────────────────────────────────────────
bool initMLX90640() {
    if (tcaselect(MUX_MLX90640, CH_MLX90640)) {
        if (mlx.begin(MLX90640_I2CADDR_DEFAULT, &Wire)) {
            mlx.setMode(MLX90640_CHESS);
            mlx.setResolution(MLX90640_ADC_18BIT);
            mlx.setRefreshRate(MLX90640_2_HZ);
            ATLAS_LOG("[HW INIT] 0x71/CH%d: MLX90640 Thermal Camera -> ONLINE\n", CH_MLX90640);
            mlx90640_ok = true; return true;
        }
    }
    ATLAS_LOG("[HW INIT] 0x71/CH%d: MLX90640 -> FAILED\n", CH_MLX90640);
    mlx90640_ok = false; return false;
}

// ─────────────────────────────────────────────────────────────────────────────
// ZMOD4510 — Renesas Industrial Gas Sensor (NO2 + O3 + EPA AQI)
// I2C: 0x33 | MUX: 0x72 CH1 | Warm-up: ~50 measurement cycles
//
// The Renesas SDK uses an internal AI/ML algorithm to extract NO2 concentration
// (ppb), O3 concentration (ppb), FAST_AQI (1-min average) and EPA_AQI (8-hr).
// Accuracy: NO2 ±0.05 ppm, O3 ±0.05 ppm @ 25°C, 50% RH
//
// HAL is ported via Wire-based callbacks bound to the Renesas Interface_t struct.
// ─────────────────────────────────────────────────────────────────────────────
bool initZMOD4510() {
    if (!tcaselect(MUX_ZMOD4510, CH_ZMOD4510)) return false;

    // Use official Renesas Arduino HAL (Wire-based, built in SDK)
    int hal_ret = HAL_Init(&zmod4510_hal);
    if (hal_ret) { ATLAS_LOG("[HW INIT] 0x72/CH%d: ZMOD4510 -> HAL INIT FAILED\n", CH_ZMOD4510); return false; }

    // Configure device
    zmod4510_dev.i2c_addr  = ZMOD4510_I2C_ADDR;
    zmod4510_dev.pid       = ZMOD4510_PID;
    zmod4510_dev.init_conf = &zmod_no2_o3_sensor_cfg[INIT];
    zmod4510_dev.meas_conf = &zmod_no2_o3_sensor_cfg[MEASUREMENT];
    zmod4510_dev.prod_data = zmod4510_prod;

    int ret = zmod4xxx_init(&zmod4510_dev, &zmod4510_hal);
    if (ret) { ATLAS_LOG("[HW INIT] 0x72/CH%d: ZMOD4510 -> I2C FAILED (err %d)\n", CH_ZMOD4510, ret); return false; }

    ret = zmod4xxx_read_sensor_info(&zmod4510_dev);
    if (ret) { ATLAS_LOG("[HW INIT] 0x72/CH%d: ZMOD4510 -> INFO READ FAILED (err %d)\n", CH_ZMOD4510, ret); return false; }

    // Run cleaning if needed (once in sensor lifetime, takes ~1 min)
    ret = zmod4xxx_cleaning_run(&zmod4510_dev);
    if (ret && ret != ERROR_CLEANING)
        ATLAS_LOG("[HW INIT] 0x72/CH%d: ZMOD4510 -> Cleaning error %d (may be ok)\n", CH_ZMOD4510, ret);

    ret = zmod4xxx_prepare_sensor(&zmod4510_dev);
    if (ret) { ATLAS_LOG("[HW INIT] 0x72/CH%d: ZMOD4510 -> PREPARE FAILED (err %d)\n", CH_ZMOD4510, ret); return false; }

    ret = init_no2_o3(&zmod4510_algo_handle);
    if (ret) { ATLAS_LOG("[HW INIT] 0x72/CH%d: ZMOD4510 -> ALGO INIT FAILED (err %d)\n", CH_ZMOD4510, ret); return false; }

    // Set default T/H inputs (will be overridden with SHT45 data during readings)
    zmod4510_input.adc_result       = zmod4510_adc;
    zmod4510_input.humidity_pct     = 50.0f;
    zmod4510_input.temperature_degc = 25.0f;

    ATLAS_LOG("[HW INIT] 0x72/CH%d: ZMOD4510 NO2+O3 (0x33) -> ONLINE (warming up ~50 cycles)\n", CH_ZMOD4510);
    zmod4510_ok = true; zmod4510_stabilizing = true;
    return true;
}

bool initRG15() {
    Serial1.begin(9600, SERIAL_8N1, RAIN_RG15_RX, RAIN_RG15_TX);
    Serial1.setTimeout(100); // Fail-fast dla funkcji czytających z UART
    
    // Wymuszenie jednostek metrycznych
    Serial1.print("m\n");
    delay(50);
    // Sprawdzenie komunikacji
    Serial1.print("r\n");
    unsigned long start = millis();
    while (millis() - start < 1500) {
        if (Serial1.available()) {
            String resp = Serial1.readStringUntil('\n');
            if (resp.indexOf("Acc") >= 0) {
                ATLAS_LOG("[HW INIT] CH[UART]: RG-15 Rain Sensor -> ONLINE\n");
                rg15_ok = true;
                return true;
            }
        }
        yield();
    }
    ATLAS_LOG("[HW INIT] CH[UART]: RG-15 Rain Sensor -> OFFLINE / TIMEOUT\n");
    rg15_ok = false;
    return false;
}

void initSensors() {
    ATLAS_LOG("\n[SYSTEM] >>> HARDWARE BUS INITIALIZATION & WAKEUP <<<\n");
    if(!i2c_mem_ok) initI2CMemory();

    if(!bme280_ok) initBME280();
    if(!bmv080_ok) initBMV080();
    if(!ina219_ok) initINA219();
    if(!ms8607_ok) initMS8607();
    if(!max17048_ok) initMAX17048();
    if(!bme688_ok) initBME688();
    if(!sht45_ok) initSHT45();
    if(!sgp41_ok) initSGP41();
    if(!tsl2591_ok) initTSL2591();
    if(!opt4048_ok) initOPT4048();
    if(!tcs34725_ok) initTCS34725();
    if(!as7343_ok) initAS7343();
    if(!veml_ok) initVEML7700();
    if(!ltr_ok) initLTR390();
    if(!as3935_ok) initAS3935();
    if(!geiger_ok) initGeiger();
    if(!scd41_ok) initSCD41();
    if(!bmp585_ok) initBMP585();
    if(!ilps_ok) initILPS22QS();
    if(!as7331_ok) initAS7331();
    if(!bmm350_ok) initBMM350();
    if(!lsm6dsox_ok) initLSM6DSOX();
    if(!lis3mdl_ok)  initLIS3MDL();
    if(!mlx90640_ok) initMLX90640();
    if(!zmod4510_ok) initZMOD4510();
    if(!rg15_ok) initRG15();
    ATLAS_LOG("----------------------------------------------------------\n");
}

// CHECKME: CZY MOZLIWY JEST WRAPPER I OPTYMALIZACJA?
void exhaustivelyReadSensors() {
    ATLAS_LOG("\n[DATA DUMP] ---> Executing Full Sensor Read Sequence\n");
    unsigned long timer = 0;
    
    if(bme280_ok && tcaselect(MUX_BME280, CH_BME280)) {
        timer = millis();
        float t = bme280.readTemperature();
        if(!isnan(t)) {
            payload["sensors"]["BME280_Enc_Temp"] = t;
            payload["sensors"]["BME280_Enc_Hum"] = bme280.readHumidity();
            payload["sensors"]["BME280_Enc_Press"] = bme280.readPressure() / 100.0F;
            ATLAS_LOG("   |- BME280  [BMS Temp]: %.2f C | Hum: %.2f %% | Press: %.1f hPa (Took: %lu ms)\n", t, (float)payload["sensors"]["BME280_Enc_Hum"], (float)payload["sensors"]["BME280_Enc_Press"], millis()-timer);
        } else bme280_ok = false; 
    }

    if(bmv080_ok && tcaselect(MUX_BMV080, CH_BMV080)) {
        timer = millis();
        Wire.beginTransmission(0x57);
        if(Wire.endTransmission() != 0) {
            bmv080_ok = false;
            ATLAS_LOG("   |- BMV080  [ERROR]: Disconnected from I2C bus!\n");
        } else {
            // Actively read from BMV080 sensor instead of checking payload
            bmv080_output_t bmv_data = {};
            if(bmv080.readSensor(&bmv_data)) {  // Library returns truthy value on success
                // Fresh sensor reading - populate all available data
                payload["sensors"]["BMV080_PM1_0"] = bmv_data.pm1_mass_concentration;
                payload["sensors"]["BMV080_PM2_5"] = bmv_data.pm2_5_mass_concentration;
                payload["sensors"]["BMV080_PM10_0"] = bmv_data.pm10_mass_concentration;
                payload["sensors"]["BMV080_Num_PM1_0"] = bmv_data.pm1_number_concentration;
                payload["sensors"]["BMV080_Num_PM2_5"] = bmv_data.pm2_5_number_concentration;
                payload["sensors"]["BMV080_Num_PM10_0"] = bmv_data.pm10_number_concentration;
                payload["sensors"]["BMV080_Obstructed"] = bmv_data.is_obstructed ? 1 : 0;
                payload["sensors"]["BMV080_Out_Of_Range"] = bmv_data.is_outside_measurement_range ? 1 : 0;
                
                float pm25 = bmv_data.pm2_5_mass_concentration;
                float pm10 = bmv_data.pm10_mass_concentration;
                int aqi = calcAQI_PM25(pm25);
                payload["sensors"]["BMV080_EPA_AQI"] = aqi;
                float pm_ratio = (pm10 > 0) ? (pm25 / pm10) * 100.0f : 0.0f;
                payload["sensors"]["BMV080_PM_Ratio_pct"] = pm_ratio;
                
                ATLAS_LOG("   |- BMV080  [Dust]: PM1.0:%.1f | PM2.5:%.1f | PM10:%.1f ug/m3 | N(2.5):%.0f/cm3 | AQI:%d | Obs:%d | OOR:%d (Took: %lu ms)\n", 
                    bmv_data.pm1_mass_concentration, pm25, pm10, 
                    bmv_data.pm2_5_number_concentration, aqi, 
                    (int)bmv_data.is_obstructed, 
                    (int)bmv_data.is_outside_measurement_range, millis()-timer);
            } else {
                // Sensor read failed - check if buffered data exists
                if (!payload["sensors"]["BMV080_PM2_5"].isNull()) {
                    float pm25 = payload["sensors"]["BMV080_PM2_5"];
                    float pm10 = payload["sensors"]["BMV080_PM10_0"];
                    int aqi = calcAQI_PM25(pm25);
                    payload["sensors"]["BMV080_EPA_AQI"] = aqi;
                    float pm_ratio = (pm10 > 0) ? (pm25 / pm10) * 100.0f : 0.0f;
                    payload["sensors"]["BMV080_PM_Ratio_pct"] = pm_ratio;
                    ATLAS_LOG("   |- BMV080  [Dust]: BUFFERED - PM1.0:%.1f | PM2.5:%.1f | PM10:%.1f ug/m3 | N(2.5):%.0f/cm3 | AQI:%d | Obs:%d | OOR:%d (Took: %lu ms)\n", 
                        (float)payload["sensors"]["BMV080_PM1_0"], pm25, pm10, 
                        (float)payload["sensors"]["BMV080_Num_PM2_5"], aqi, 
                        (int)payload["sensors"]["BMV080_Obstructed"], 
                        (int)payload["sensors"]["BMV080_Out_Of_Range"], millis()-timer);
                } else {
                    ATLAS_LOG("   |- BMV080  [Dust]: Calculating / Laser buffering... (Took: %lu ms)\n", millis()-timer);
                }
            }
        }
    }

    global_solar_mw = 0.0f;
	if(ina219_ok && tcaselect(MUX_INA219, CH_INA219)) {
        timer = millis();
        float bus_v = ina219.getBusVoltage_V();
        if (bus_v < 0) { bus_v += 32.768f; }
        
        float shunt_mv = ina219.getShuntVoltage_mV();
        float cur_ma = ina219.getCurrent_mA();
        global_solar_mw = ina219.getPower_mW();

		// ---- tu jest kolejny glitch odczytow --->
		// [BUG_HW#0001]
		// UWAGA, PROBLEMY Z GND, SPRAWDZIC HARDWARE
		// ZMIENIC SYSTEM ZASILANIA NA DFROBOT dfr0535
		// ZASTANOWIC SIE NAD IMPLEMENTACJA FILTRU WYCINAJACEGO NIECHCIANE 
		// PEAKI JAKO WORKAROUND SOFTWARE'owy, ale ostroznie
		// TO TYLKO MASKOWANIE PROBLEMU NIE JEGO ROZWIAZANIE
        if (bus_v > 35.0f || global_solar_mw > 15000.0f || cur_ma > 2000.0f || cur_ma < -500.0f) {
            ATLAS_LOG("\n[!!! GLITCH GUARD] INA219 zglosil anomalię: V:%.2f, mA:%.2f, mW:%.2f. Tlumienie!\n", bus_v, cur_ma, global_solar_mw);
            bus_v = 0.0f; shunt_mv = 0.0f; cur_ma = 0.0f; global_solar_mw = 0.0f;
        }

        float load_v = bus_v + (shunt_mv / 1000.0f);
        float efficiency = (global_solar_mw / SOLAR_PANEL_MAX_MW) * 100.0f;
        
        payload["sensors"]["SOLAR_Bus_Voltage"] = bus_v;
        payload["sensors"]["SOLAR_Load_Voltage"] = load_v;
        payload["sensors"]["SOLAR_Shunt_mV"] = shunt_mv;
        payload["sensors"]["SOLAR_Current_mA"] = cur_ma;
        payload["sensors"]["SOLAR_Power_mW"] = global_solar_mw;
        payload["sensors"]["SOLAR_Efficiency_Pct"] = efficiency;
        
        ATLAS_LOG("   |- INA219  [Solar]: Bus: %.2fV | Load: %.2fV | Cur: %.2f mA | Pow: %.2f mW | Eff: %.1f%% (Took: %lu ms)\n", bus_v, load_v, cur_ma, global_solar_mw, efficiency, millis()-timer);
    }

    if((sht45_ok || bme280_ok) && (bmp585_ok || bme280_ok)) {
        timer = millis();
        // MS8607 REPLACEMENT: Use SHT45 (lab-grade T/H) + BMP585 (precision press) + BME280 (backup)
        
        float ext_temp = -999.0f, ext_hum = -999.0f, ext_press = -999.0f;
        bool has_data = false;
        
        // Preferred: SHT45 for temperature/humidity (lab-grade accuracy ±1.5%)
        if(sht45_ok && tcaselect(MUX_SHT45, CH_SHT45)) {
            sensors_event_t hum, temp;
            if(sht45.getEvent(&hum, &temp)) {
                ext_temp = temp.temperature;
                ext_hum = hum.relative_humidity;
                has_data = true;
            }
        }
        
        // Preferred: BMP585 for pressure (ultra-precise ±5 Pa)
        if(bmp585_ok && tcaselect(MUX_BMP585, CH_BMP585)) {
            float press_raw = bmp585.readPressure() / 100.0F;
            if(press_raw > 0 && press_raw < 1200) {  // Valid range check
                ext_press = press_raw;
                has_data = true;
            }
        }
        
        // Fallback: BME280 for any missing data
        if((ext_temp < -100 || isnan(ext_temp)) && bme280_ok && tcaselect(MUX_BME280, CH_BME280)) {
            float t = bme280.readTemperature();
            if(!isnan(t) && t > -50 && t < 85) {
                ext_temp = t;
            }
        }
        if((ext_hum < 0 || isnan(ext_hum)) && bme280_ok && tcaselect(MUX_BME280, CH_BME280)) {
            float h = bme280.readHumidity();
            if(!isnan(h) && h >= 0 && h <= 100) {
                ext_hum = h;
            }
        }
        if((ext_press < 0 || isnan(ext_press)) && bme280_ok && tcaselect(MUX_BME280, CH_BME280)) {
            float p = bme280.readPressure() / 100.0F;
            if(!isnan(p) && p > 0 && p < 1200) {
                ext_press = p;
            }
        }
        
        if(has_data) {
            // All METEO calculations using combined sensor data (SHT45+BMP585)
            // Note: MS8607 virtual sensor disabled - using direct sensor keys
            payload["sensors"]["ENV_Ext_Temp"] = ext_temp;  // Combined from SHT45/BME280
            payload["sensors"]["ENV_Ext_Press"] = ext_press;  // Combined from BMP585/BME280
            payload["sensors"]["ENV_Ext_Hum"] = ext_hum;  // Combined from SHT45/BME280
            
            float dp = calcDewPoint(ext_temp, ext_hum);
            float cb = calcCloudBase(ext_temp, dp);
            float ah = calcAbsoluteHumidity(ext_temp, ext_hum);
            float slp = calcSLP(ext_press, ext_temp, STATION_ALTITUDE_METERS);
            float hi = calcHeatIndex(ext_temp, ext_hum);
            float ad = calcAirDensity(ext_press, ext_temp);
            float wbt = calcWetBulb(ext_temp, ext_hum);
            int mold_risk = (ext_hum > 70.0f && ext_temp > 15.0f && ext_temp < 30.0f) ? 1 : 0;
            int virus_risk = (ah >= 4.0f && ah <= 6.0f) ? 0 : 1;

            payload["sensors"]["METEO_Dew_Point_C"] = dp;
            payload["sensors"]["METEO_Cloud_Base_m"] = cb;
            payload["sensors"]["METEO_Abs_Hum_g_m3"] = ah;
            payload["sensors"]["METEO_Sea_Level_Press_hPa"] = slp;
            payload["sensors"]["METEO_Heat_Index"] = hi;
            payload["sensors"]["METEO_Air_Density"] = ad;
            payload["sensors"]["METEO_Mold_Risk"] = mold_risk;
            payload["sensors"]["METEO_Virus_Risk"] = virus_risk;
            payload["sensors"]["METEO_Wet_Bulb_C"] = wbt;

            // NOAA official feels-like: uses wind from WU fetch (0 if not yet available)
            float wind_for_fl = wu_wind_fetched ? wu_wind_speed : 0.0f;
            float feels_like  = calcFeelsLike(ext_temp, ext_hum, wind_for_fl);
            payload["sensors"]["METEO_Feels_Like_C"] = feels_like;

            ATLAS_LOG("   |- ENV* [Weather from SHT45+BMP585]: T:%.2fC H:%.2f%% P:%.2fhPa | DP:%.2fC WBT:%.2fC HI:%.2fC | Cloud:%.0fm AbsH:%.2fg/m3 Dens:%.2f | Mold:%d Virus:%d (Took: %lu ms)\n", 
                ext_temp, ext_hum, ext_press, dp, wbt, hi, cb, ah, ad, mold_risk, virus_risk, millis()-timer);
        } else {
            ATLAS_LOG("   |- ENV* [Weather]: No data from SHT45+BMP585+BME280 sensors!\n");
        }
    }
        
        
    if(bme688_ok && tcaselect(MUX_BME688, CH_BME688)) {
            if (!payload["sensors"]["BME688_IAQ"].isNull()) {
                float iaq = payload["sensors"]["BME688_IAQ"];
                String iaqStatus = "No data";
                if (iaq <= 50) iaqStatus = "Excellent";
                else if (iaq <= 100) iaqStatus = "Good";
                else if (iaq <= 150) iaqStatus = "Moderate";
                else if (iaq <= 200) iaqStatus = "Poor";
                else if (iaq <= 300) iaqStatus = "Very Poor";
                else iaqStatus = "Hazardous";
                payload["sensors"]["BME688_IAQ_Status"] = iaqStatus;
                ATLAS_LOG("   |- BME688  [AI Brain]: IAQ: %.0f (%s) | eCO2: %.0f ppm | bVOC: %.2f ppm | Gas: %.0f Ohm | Acc: %d\n", 
                iaq, iaqStatus.c_str(), (float)payload["sensors"]["BME688_eCO2"], (float)payload["sensors"]["BME688_bVOC"], 
                    (float)payload["sensors"]["BME688_Gas_Res_Raw"], (int)payload["sensors"]["BME688_IAQ_Accuracy"]);
            } else ATLAS_LOG("   |- BME688  [AI Brain]: BSEC Algorithm stabilizing...\n");
        }

	if(sht45_ok && tcaselect(MUX_SHT45, CH_SHT45)) {
            timer = millis();
            sensors_event_t hum, temp;
            if(sht45.getEvent(&hum, &temp)) {
                float t = temp.temperature;
                float h = hum.relative_humidity;
                
                // kolejny glitch kurwa, naprawic hardware?
				// [BUG_HW#0001]
				// UWAGA, PROBLEMY Z GND, SPRAWDZIC HARDWARE
				// ZMIENIC SYSTEM ZASILANIA NA DFROBOT dfr0535
				// ZASTANOWIC SIE NAD IMPLEMENTACJA FILTRU WYCINAJACEGO NIECHCIANE 
				// PEAKI JAKO WORKAROUND SOFTWARE'owy, ale ostroznie
				// TO TYLKO MASKOWANIE PROBLEMU NIE JEGO ROZWIAZANIE
                if (t < -50.0f || t > 85.0f || h < 0.0f || h > 100.0f) {
                    ATLAS_LOG("\n[!!! GLITCH GUARD] SHT45 zglosil anomalię fizyczna: Temp: %.2fC, Hum: %.2f%%. Ignorowanie odczytu!\n", t, h);
                } else {
                    payload["sensors"]["SHT45_Temp"] = t;
                    payload["sensors"]["SHT45_Hum"] = h;
                    ATLAS_LOG("   |- SHT45   [Clima]: T:%.2fC H:%.2f%% (Took: %lu ms)\n", t, h, millis()-timer);
                }
            } else sht45_ok = false;
        }
        
    if(sgp41_ok && tcaselect(MUX_SGP41, CH_SGP41)) {
            timer = millis();
            payload["sensors"]["SGP41_Raw_VOC"] = current_sraw_voc;
            payload["sensors"]["SGP41_Raw_NOx"] = current_sraw_nox;
            payload["sensors"]["SGP41_VOC_Index"] = current_voc_index;
            payload["sensors"]["SGP41_NOx_Index"] = current_nox_index;
            
            String vocStatus = "Normal";
            if (current_voc_index < 100) vocStatus = "Very Clean";
            else if (current_voc_index > 150) vocStatus = "Odor / VOC Detected";
            String noxStatus = "No Exhaust";
            if (current_nox_index > 15 && current_nox_index <= 50) noxStatus = "Light Exhaust";
            else if (current_nox_index > 50) noxStatus = "Heavy Exhaust / Smog";
            
            payload["sensors"]["SGP41_VOC_Status"] = vocStatus;
            payload["sensors"]["SGP41_NOx_Status"] = noxStatus;
            ATLAS_LOG("   |- SGP41   [Gas AI]: VOC Idx:%d (%s) | NOx Idx:%d (%s) (Took: %lu ms)\n", current_voc_index, vocStatus.c_str(), current_nox_index, noxStatus.c_str(), millis()-timer);
        }
        
    if(tsl2591_ok && tcaselect(MUX_TSL2591, CH_TSL2591)) {
            timer = millis();
            uint32_t lum = tsl2591.getFullLuminosity();
            uint16_t ir, full;
            ir = lum >> 16;
            full = lum & 0xFFFF;
            float lux = tsl2591.calculateLux(full, ir);
            payload["sensors"]["TSL2591_Lux"] = lux;
            payload["sensors"]["TSL2591_Visible"] = full - ir;
            payload["sensors"]["TSL2591_IR"] = ir;
            ATLAS_LOG("   |- TSL2591 [Light]: %.2f Lux | Vis:%u | IR:%u (Took: %lu ms)\n", lux, full-ir, ir, millis()-timer);
        }

   if(opt4048_ok && tcaselect(MUX_OPT4048, CH_OPT4048)) {
            timer = millis();
            double cie_x = 0, cie_y = 0, lux = 0;
            if (opt4048.getCIE(&cie_x, &cie_y, &lux)) {
                payload["sensors"]["OPT4048_CIE_X"] = cie_x;
                payload["sensors"]["OPT4048_CIE_Y"] = cie_y;
                payload["sensors"]["OPT4048_Lux"] = lux;
                ATLAS_LOG("   |- OPT4048 [Sky Color]: CIE x:%.4f y:%.4f Lux:%.2f (Took: %lu ms)\n", cie_x, cie_y, lux, millis()-timer);
            } else {
                ATLAS_LOG("   |- OPT4048 [Sky Color]: Read failed (Took: %lu ms)\n", millis()-timer);
            }
        }
        
    if(tcs34725_ok && tcaselect(MUX_TCS34725, CH_TCS34725)) {
            timer = millis();
            uint16_t r, g, b, c;
            tcs34725.getRawData(&r, &g, &b, &c);
            
            // Pełna matematyka środowiskowa TCS
            uint16_t colorTemp = tcs34725.calculateColorTemperature(r, g, b);
            uint16_t lux = tcs34725.calculateLux(r, g, b);
            
            payload["sensors"]["TCS34725_R"] = r;
            payload["sensors"]["TCS34725_G"] = g;
            payload["sensors"]["TCS34725_B"] = b;
            payload["sensors"]["TCS34725_C"] = c;
            payload["sensors"]["TCS34725_ColorTemp_K"] = colorTemp;
            payload["sensors"]["TCS34725_Lux"] = lux;
            
            ATLAS_LOG("   |- TCS34725[RGB Light]: R:%d G:%d B:%d C:%d | CCT:%dK | Lux:%d (Took: %lu ms)\n", r, g, b, c, colorTemp, lux, millis()-timer);
        }
        
    if(bmm350_ok && tcaselect(MUX_BMM350, CH_BMM350)) {
            timer = millis();
            sBmm350MagData_t mag = bmm350.getGeomagneticData();
            if(mag.x != 0 || mag.y != 0 || mag.z != 0) {
                float heading = bmm350.getCompassDegree();
                payload["sensors"]["BMM350_Mag_X"] = mag.x;
                payload["sensors"]["BMM350_Mag_Y"] = mag.y;
                payload["sensors"]["BMM350_Mag_Z"] = mag.z;
                payload["sensors"]["BMM350_Heading_Deg"] = heading;
                
                // Calculate cardinal direction
                String cardinal;
                if(heading >= 337.5 || heading < 22.5) cardinal = "N";
                else if(heading < 67.5) cardinal = "NE";
                else if(heading < 112.5) cardinal = "E";
                else if(heading < 157.5) cardinal = "SE";
                else if(heading < 202.5) cardinal = "S";
                else if(heading < 247.5) cardinal = "SW";
                else if(heading < 292.5) cardinal = "W";
                else cardinal = "NW";
                payload["sensors"]["BMM350_Cardinal"] = cardinal;
                
                ATLAS_LOG("   |- BMM350 [Magnetometer]: X:%.2f Y:%.2f Z:%.2f uT | Heading: %.1f° %s (Took: %lu ms)\n", 
                    mag.x, mag.y, mag.z, heading, cardinal.c_str(), millis()-timer);
            } else {
                ATLAS_LOG("   |- BMM350 [Magnetometer]: Read failed (Took: %lu ms)\n", millis()-timer);
            }
        }

    // -----------------------------------------------------------------------
    // LSM6DSOX — 6-axis IMU: acceleration (m/s²) + gyroscope (rad/s) + temp
    // Also computes: tilt angle, vibration magnitude, shock detection
    // -----------------------------------------------------------------------
    if(lsm6dsox_ok && tcaselect(MUX_LSM6DSOX, CH_LSM6DSOX)) {
        timer = millis();
        sensors_event_t accel, gyro, temp_imu;
        if(lsm6dsox.getEvent(&accel, &gyro, &temp_imu)) {
            float ax = accel.acceleration.x;
            float ay = accel.acceleration.y;
            float az = accel.acceleration.z;
            float gx = gyro.gyro.x;   // rad/s
            float gy = gyro.gyro.y;
            float gz = gyro.gyro.z;

            // Total acceleration magnitude (m/s²)
            float accel_mag = sqrtf(ax*ax + ay*ay + az*az);
            // Tilt from vertical (degrees) — useful for sensor leveling
            float tilt_deg  = acosf(az / accel_mag) * 180.0f / M_PI;
            // Gyro magnitude (total rotation rate, rad/s)
            float gyro_mag  = sqrtf(gx*gx + gy*gy + gz*gz);
            // Shock: accel significantly above 1g baseline
            bool shock = (accel_mag > 15.0f);
            // Vibration proxy: deviation from 9.81 m/s²
            float vibration = fabsf(accel_mag - 9.81f);

            payload["sensors"]["LSM6_Accel_X"]       = roundf(ax * 1000.0f) / 1000.0f;
            payload["sensors"]["LSM6_Accel_Y"]       = roundf(ay * 1000.0f) / 1000.0f;
            payload["sensors"]["LSM6_Accel_Z"]       = roundf(az * 1000.0f) / 1000.0f;
            payload["sensors"]["LSM6_Accel_Mag"]     = roundf(accel_mag * 1000.0f) / 1000.0f;
            payload["sensors"]["LSM6_Gyro_X_rads"]   = roundf(gx * 10000.0f) / 10000.0f;
            payload["sensors"]["LSM6_Gyro_Y_rads"]   = roundf(gy * 10000.0f) / 10000.0f;
            payload["sensors"]["LSM6_Gyro_Z_rads"]   = roundf(gz * 10000.0f) / 10000.0f;
            payload["sensors"]["LSM6_Gyro_Mag_rads"] = roundf(gyro_mag * 10000.0f) / 10000.0f;
            payload["sensors"]["LSM6_Temp_C"]        = roundf(temp_imu.temperature * 100.0f) / 100.0f;
            payload["sensors"]["LSM6_Tilt_Deg"]      = roundf(tilt_deg * 10.0f) / 10.0f;
            payload["sensors"]["LSM6_Vibration"]     = roundf(vibration * 1000.0f) / 1000.0f;
            payload["sensors"]["LSM6_Shock_Det"]     = shock ? 1 : 0;

            ATLAS_LOG("   |- LSM6DSOX [IMU]: A=%.2f/%.2f/%.2f m/s² (|%.2f|) G=%.3f/%.3f/%.3f rad/s Tilt:%.1f° Vib:%.3f T:%.1f°C (Took: %lu ms)\n",
                ax, ay, az, accel_mag, gx, gy, gz, tilt_deg, vibration, temp_imu.temperature, millis()-timer);
        } else {
            ATLAS_LOG("   |- LSM6DSOX [IMU]: Read failed (Took: %lu ms)\n", millis()-timer);
        }
    }

    // -----------------------------------------------------------------------
    // LIS3MDL — 3-axis magnetometer: X/Y/Z (µT) + field magnitude + heading
    // Combined with LSM6DSOX to form a full 9-DOF AHRS-capable cluster
    // Separate from BMM350 (which is the precision met/space weather sensor)
    // -----------------------------------------------------------------------
    if(lis3mdl_ok && tcaselect(MUX_LIS3MDL, CH_LIS3MDL)) {
        timer = millis();
        sensors_event_t mag_ev;
        if(lis3mdl.getEvent(&mag_ev)) {
            float mx = mag_ev.magnetic.x;  // µT
            float my = mag_ev.magnetic.y;
            float mz = mag_ev.magnetic.z;
            float field_mag = sqrtf(mx*mx + my*my + mz*mz);
            // Compass heading from X/Y components (flat-level assumption)
            float heading_lis = atan2f(my, mx) * 180.0f / M_PI;
            if(heading_lis < 0) heading_lis += 360.0f;

            payload["sensors"]["LIS3MDL_X_uT"]      = roundf(mx * 100.0f) / 100.0f;
            payload["sensors"]["LIS3MDL_Y_uT"]      = roundf(my * 100.0f) / 100.0f;
            payload["sensors"]["LIS3MDL_Z_uT"]      = roundf(mz * 100.0f) / 100.0f;
            payload["sensors"]["LIS3MDL_Mag_uT"]    = roundf(field_mag * 100.0f) / 100.0f;
            payload["sensors"]["LIS3MDL_Heading"]   = roundf(heading_lis * 10.0f) / 10.0f;

            ATLAS_LOG("   |- LIS3MDL  [Mag]: X:%.2f Y:%.2f Z:%.2f µT |F|:%.2f µT Heading:%.1f° (Took: %lu ms)\n",
                mx, my, mz, field_mag, heading_lis, millis()-timer);
        } else {
            ATLAS_LOG("   |- LIS3MDL  [Mag]: Read failed (Took: %lu ms)\n", millis()-timer);
        }
    }

    // ─────────────────────────────────────────────────────────────────────────
    // ZMOD4510 — Renesas Industrial NO2 + O3 (official SDK)
    // Uses external SHT45 T/H for algorithm compensation
    // Outputs: NO2_conc_ppb, O3_conc_ppb, FAST_AQI, EPA_AQI
    // ─────────────────────────────────────────────────────────────────────────
    if(zmod4510_ok && tcaselect(MUX_ZMOD4510, CH_ZMOD4510)) {
        timer = millis();

        // Update T/H from SHT45 for better algorithm accuracy
        if (!payload["sensors"]["SHT45_Temp"].isNull())
            zmod4510_input.temperature_degc = payload["sensors"]["SHT45_Temp"];
        if (!payload["sensors"]["SHT45_Hum"].isNull())
            zmod4510_input.humidity_pct = payload["sensors"]["SHT45_Hum"];

        int ret_m = zmod4xxx_start_measurement(&zmod4510_dev);
        if (ret_m == ZMOD4XXX_OK) {
            zmod4510_dev.delay_ms(ZMOD4510_NO2_O3_SAMPLE_TIME);

            uint8_t zmod_status;
            zmod4xxx_read_status(&zmod4510_dev, &zmod_status);
            if (!(zmod_status & STATUS_SEQUENCER_RUNNING_MASK)) {
                ret_m = zmod4xxx_read_adc_result(&zmod4510_dev, zmod4510_adc);
                if (ret_m == ZMOD4XXX_OK) {
                    int8_t ret_calc = calc_no2_o3(&zmod4510_algo_handle, &zmod4510_dev,
                                                   &zmod4510_input, &zmod4510_results);
                    if (ret_calc == NO2_O3_OK) {
                        zmod4510_stabilizing = false;
                        // ppb → µg/m³ conversions @ 25°C, 1013 hPa:
                        // NO2: 1 ppb = 1.913 µg/m³; O3: 1 ppb = 1.996 µg/m³
                        float no2_ppb = zmod4510_results.NO2_conc_ppb;
                        float o3_ppb  = zmod4510_results.O3_conc_ppb;

                        payload["sensors"]["ZMOD4510_NO2_ppb"]   = roundf(no2_ppb  * 100.0f) / 100.0f;
                        payload["sensors"]["ZMOD4510_O3_ppb"]    = roundf(o3_ppb   * 100.0f) / 100.0f;
                        payload["sensors"]["ZMOD4510_NO2_ugm3"]  = roundf(no2_ppb  * 1.913f * 10.0f) / 10.0f;
                        payload["sensors"]["ZMOD4510_O3_ugm3"]   = roundf(o3_ppb   * 1.996f * 10.0f) / 10.0f;
                        payload["sensors"]["ZMOD4510_FAST_AQI"]  = (int)zmod4510_results.FAST_AQI;
                        payload["sensors"]["ZMOD4510_EPA_AQI"]   = (int)zmod4510_results.EPA_AQI;

                        // WHO O3 limits: 8h average > 100 µg/m³ = harmful
                        float o3_ugm3 = o3_ppb * 1.996f;
                        String o3_risk = o3_ugm3 > 180.0f ? "Hazardous" :
                                         o3_ugm3 > 120.0f ? "High" :
                                         o3_ugm3 > 100.0f ? "Moderate" : "Low";
                        payload["sensors"]["ZMOD4510_O3_Risk"]   = o3_risk;

                        // Summer smog oxidative stress index (NO2+O3 combined)
                        float smog_summer = (no2_ppb / 100.0f) * 40.0f + (o3_ppb / 70.0f) * 60.0f;
                        payload["sensors"]["ZMOD4510_Smog_Index"]= roundf(min(100.0f, smog_summer));

                        ATLAS_LOG("   |- ZMOD4510 [NO2+O3]: NO2:%.2fppb O3:%.2fppb FAQI:%d EAQI:%d (Took: %lu ms)\n",
                            no2_ppb, o3_ppb, zmod4510_results.FAST_AQI, zmod4510_results.EPA_AQI, millis()-timer);

                    } else if (ret_calc == NO2_O3_STABILIZATION) {
                        payload["sensors"]["ZMOD4510_Status"] = "Stabilizing";
                        ATLAS_LOG("   |- ZMOD4510 [NO2+O3]: Warming up (Took: %lu ms)\n", millis()-timer);
                    } else {
                        ATLAS_LOG("   |- ZMOD4510 [NO2+O3]: Algo error %d (Took: %lu ms)\n", ret_calc, millis()-timer);
                        if (ret_calc == NO2_O3_DAMAGE) {
                            payload["sensors"]["Fault_ZMOD4510"] = "ON";
                            zmod4510_ok = false;
                        }
                    }
                }
            }
        }
    }

    // ─────────────────────────────────────────────────────────────────────────
    // MLX90640 — Thermal IR sky scan for cloud cover detection & sky temp
    // Pixel temperatures analysed:
    //   < -10°C = likely open sky atmospheric window (clear sky cold sink)
    //   > -10°C = cloud canopy or warm object (overcast / partial clouds)
    // Cloud cover % = fraction of pixels warmer than -10°C
    // Sky temp (clear-sky pixels average) → see "cosmic window" 230K
    // ─────────────────────────────────────────────────────────────────────────
    if(mlx90640_ok && tcaselect(MUX_MLX90640, CH_MLX90640)) {
        timer = millis();
        static float frame[32*24];
        if(mlx.getFrame(frame) == 0) {
            int total_px = 32*24;
            int cloud_px = 0;
            float sky_sum = 0; int sky_cnt = 0;
            float min_px = 999, max_px = -999;
            for(int i = 0; i < total_px; i++) {
                float px = frame[i];
                if(px < min_px) min_px = px;
                if(px > max_px) max_px = px;
                if(px > -10.0f) cloud_px++;  // warm = cloud/warm object
                else { sky_sum += px; sky_cnt++; }
            }
            float cloud_cover_pct = (float)cloud_px / total_px * 100.0f;
            float sky_temp_c      = (sky_cnt > 0) ? sky_sum / sky_cnt : -999.0f;
            float scene_min       = min_px;
            float scene_max       = max_px;

            // Night-sky discrimination: true clear sky should have sky_temp < -25°C
            String sky_cond;
            if (cloud_cover_pct < 10.0f)       sky_cond = "Clear";
            else if (cloud_cover_pct < 30.0f)  sky_cond = "Few Clouds";
            else if (cloud_cover_pct < 60.0f)  sky_cond = "Partly Cloudy";
            else if (cloud_cover_pct < 90.0f)  sky_cond = "Mostly Cloudy";
            else                               sky_cond = "Overcast";

            payload["sensors"]["MLX_Cloud_Cover_Pct"] = roundf(cloud_cover_pct * 10.0f) / 10.0f;
            payload["sensors"]["MLX_Sky_Temp_C"]      = roundf(sky_temp_c * 10.0f) / 10.0f;
            payload["sensors"]["MLX_Scene_Min_C"]     = roundf(scene_min * 10.0f) / 10.0f;
            payload["sensors"]["MLX_Scene_Max_C"]     = roundf(scene_max * 10.0f) / 10.0f;
            payload["sensors"]["MLX_Sky_Condition"]   = sky_cond;

            // Aurora window: clear sky at night is required for visibility
            bool night = !payload["sensors"]["TSL2591_Lux"].isNull()
                         && (float)payload["sensors"]["TSL2591_Lux"] < 1.0f;
            payload["sensors"]["SPACE_Aurora_Sky_Clear"] =
                (cloud_cover_pct < 30.0f && night) ? "YES" : "No";

            ATLAS_LOG("   |- MLX90640 [Sky]: Coverage:%.1f%% (%s) SkyT:%.1f°C Range:%.1f~%.1f°C (Took: %lu ms)\n",
                cloud_cover_pct, sky_cond.c_str(), sky_temp_c, scene_min, scene_max, millis()-timer);
        } else {
            ATLAS_LOG("   |- MLX90640 [Sky]: Frame read failed (Took: %lu ms)\n", millis()-timer);
        }
    }

    if(i2c_mem_ok && tcaselect(MUX_I2CMEM, CH_I2CMEM)) {
            Wire.beginTransmission(0x50);
            if(Wire.endTransmission() != 0) i2c_mem_ok = false;
        }

    if(max17048_ok && tcaselect(MUX_MAX17048, CH_MAX17048)) {
            timer = millis();
            float v_peak = bat.cellVoltage();
            float soc = bat.cellPercent(); // Odczytujemy SOC przed walidacją
            float crate_peak = bat.chargeRate(); 
            
			// ---- tu jest kolejny glitch odczytow --->
			// [BUG_HW#0001]
			// UWAGA, PROBLEMY Z GND, SPRAWDZIC HARDWARE
			// ZMIENIC SYSTEM ZASILANIA NA DFROBOT dfr0535
			// ZASTANOWIC SIE NAD IMPLEMENTACJA FILTRU WYCINAJACEGO NIECHCIANE 
			// PEAKI JAKO WORKAROUND SOFTWARE'owy, ale ostroznie
			// TO TYLKO MASKOWANIE PROBLEMU NIE JEGO ROZWIAZANIE
            if (!isnan(v_peak) && v_peak >= 2.0f && v_peak <= 5.0f && !isnan(soc) && soc >= 0.0f && soc <= 110.0f) {
                
                if (crate_peak > 50.0f || crate_peak < -50.0f) {
                    ATLAS_LOG("\n[!!! GLITCH GUARD] MAX17048 anomalia drenazu: %.2f%%/h. Tlumienie do 0.0!\n", crate_peak);
                    crate_peak = 0.0f;
                }
                
                if (!baseline_taken) {
                    baseline_charge_rate = crate_peak;
                    baseline_voltage = v_peak; 
                }

                float avg_crate = ((baseline_charge_rate * 45.0f) + (crate_peak * 15.0f)) / 60.0f;
                float avg_voltage = ((baseline_voltage * 45.0f) + (v_peak * 15.0f)) / 60.0f;
                
                float remaining_mah = (soc / 100.0f) * BATTERY_CAPACITY_MAH;
                float net_bat_ma = (avg_crate / 100.0f) * BATTERY_CAPACITY_MAH; 
                float net_bat_mw = net_bat_ma * avg_voltage;
                
                float esp_power_mw = global_solar_mw - net_bat_mw;
                if (esp_power_mw < 0) esp_power_mw = 0.0f;
                float esp_current_ma = esp_power_mw / avg_voltage;
                
                float solar_utilization_pct = 0.0f;
                if (global_solar_mw > 0.01f) solar_utilization_pct = (esp_power_mw / global_solar_mw) * 100.0f;
                float esp_vs_max_pct = (esp_power_mw / SOLAR_PANEL_MAX_MW) * 100.0f;
                float time_empty_min = 99999.0f;
                if (net_bat_ma < -0.1f) time_empty_min = (remaining_mah / abs(net_bat_ma)) * 60.0f;

                float time_full_cur_min = 99999.0f;
                if (net_bat_ma > 0.1f) time_full_cur_min = ((BATTERY_CAPACITY_MAH - remaining_mah) / net_bat_ma) * 60.0f;
                float time_full_100_min = ((BATTERY_CAPACITY_MAH - remaining_mah) / 2000.0f) * 60.0f; 

                payload["sensors"]["BMS_Cell_Voltage"] = avg_voltage;
                payload["sensors"]["BMS_State_Of_Charge"] = soc;
                payload["sensors"]["BMS_Charge_Rate"] = avg_crate;
                payload["sensors"]["BMS_Remaining_Capacity_mAh"] = remaining_mah;
                payload["sensors"]["BMS_ESP_Power_mW"] = esp_power_mw;
                payload["sensors"]["BMS_ESP_Current_mA"] = esp_current_ma;
                payload["sensors"]["BMS_Time_To_Empty_min"] = time_empty_min;
                payload["sensors"]["BMS_Time_To_Full_Current_min"] = time_full_cur_min;
                payload["sensors"]["BMS_Time_To_Full_100_min"] = time_full_100_min;
                payload["sensors"]["SOLAR_ESP_Utilization_pct"] = solar_utilization_pct;
                payload["sensors"]["SOLAR_ESP_vs_max_pct"] = esp_vs_max_pct;
                
                ATLAS_LOG("   |- MAX17048[Battery]: SOC: %.1f%% | Avg %.2fV | ESP Load: %.1fmA | Empty: %.1fh | Full: %.1fh (Took: %lu ms)\n", 
                    soc, avg_voltage, esp_current_ma, time_empty_min/60.0f, time_full_cur_min/60.0f, millis()-timer);
            } else {
                ATLAS_LOG("   |- MAX17048[Battery]: [ERROR] Dane z kosmosu (V: %.2fV, SOC: %.1f%%) - Odrzucam!\n", v_peak, soc);
                max17048_ok = false;
            }
        }

    if(as7343_ok && tcaselect(MUX_AS7343, CH_AS7343)) {
            timer = millis();
            if(as7343.readSpectraDataFromSensor()) {
                uint16_t raw_clear = as7343.getChannelData(CH_VIS_1);
                uint16_t max_val = raw_clear; // Clear zazwyczaj łapie najwięcej fotonów
                
                // --- 1. LOGIKA AUTO-GAIN (Zarządzanie na następny cykl) ---
                uint16_t sw_multiplier = 1;
                
                if (max_val > 60000 && optics_gain_idx > 0) {
                    optics_gain_idx--;
                    as7343.disableSpectralMeasurement();
                    as7343.setAgain(as_gain_values[optics_gain_idx]); 
                    as7343.enableSpectralMeasurement();
                    ATLAS_LOG("   [AGC] Saturation detected. Decreasing hardware gain.\n");
                } 
                else if (max_val < 500 && optics_gain_idx < 12) {
                    optics_gain_idx++;
                    as7343.disableSpectralMeasurement();
                    as7343.setAgain(as_gain_values[optics_gain_idx]); 
                    as7343.enableSpectralMeasurement();
                    ATLAS_LOG("   [AGC] Weak signal. Increasing hardware gain.\n");
                }
                else if (max_val < 20 && optics_gain_idx == 12) {
                    sw_multiplier = 8;
                }

                float hw_mult = as_gain_multipliers[optics_gain_idx];
                float total_mult = hw_mult * sw_multiplier;

                // --- 2. ODCZYT I APLIKACJA MNOŻNIKA ---
                opt_data.clear = raw_clear * sw_multiplier;
                opt_data.f1  = as7343.getChannelData(CH_PURPLE_F1_405NM) * sw_multiplier;
                opt_data.f2  = as7343.getChannelData(CH_DARK_BLUE_F2_425NM) * sw_multiplier;
                opt_data.fz  = as7343.getChannelData(CH_BLUE_FZ_450NM) * sw_multiplier; // Nowy
                opt_data.f3  = as7343.getChannelData(CH_LIGHT_BLUE_F3_475NM) * sw_multiplier;
                opt_data.f4  = as7343.getChannelData(CH_BLUE_F4_515NM) * sw_multiplier;
                opt_data.f5  = as7343.getChannelData(CH_GREEN_F5_550NM) * sw_multiplier;
                opt_data.fy  = as7343.getChannelData(CH_GREEN_FY_555NM) * sw_multiplier; // Nowy
                opt_data.fxl = as7343.getChannelData(CH_ORANGE_FXL_600NM) * sw_multiplier; // Nowy
                opt_data.f6  = as7343.getChannelData(CH_BROWN_F6_640NM) * sw_multiplier;
                opt_data.f7  = as7343.getChannelData(CH_RED_F7_690NM) * sw_multiplier;
                opt_data.f8  = as7343.getChannelData(CH_DARK_RED_F8_745NM) * sw_multiplier;
                opt_data.nir = as7343.getChannelData(CH_NIR_855NM) * sw_multiplier;
                opt_data.fd  = as7343.getChannelData(CH_FD_1) * sw_multiplier; // Nowy (Flicker)
                
                // Zapis danych do JSON (nowe klucze AS7343 z 14 kanałami)
                payload["sensors"]["AS7343_F1_405nm"] = opt_data.f1; payload["sensors"]["AS7343_F2_425nm"] = opt_data.f2;
                payload["sensors"]["AS7343_FZ_450nm"] = opt_data.fz; payload["sensors"]["AS7343_F3_475nm"] = opt_data.f3;
                payload["sensors"]["AS7343_F4_515nm"] = opt_data.f4; payload["sensors"]["AS7343_F5_550nm"] = opt_data.f5;
                payload["sensors"]["AS7343_FY_555nm"] = opt_data.fy; payload["sensors"]["AS7343_FXL_600nm"] = opt_data.fxl;
                payload["sensors"]["AS7343_F6_640nm"] = opt_data.f6; payload["sensors"]["AS7343_F7_690nm"] = opt_data.f7;
                payload["sensors"]["AS7343_F8_745nm"] = opt_data.f8; payload["sensors"]["AS7343_Clear"] = opt_data.clear;
                payload["sensors"]["AS7343_NIR"] = opt_data.nir;
                payload["sensors"]["AS7343_Flicker"] = opt_data.fd;
                
                payload["sensors"]["OPTICS_AS7343_Gain"] = total_mult;
                
                float integration_correction = 60.0f; 
                float n_f1 = (opt_data.f1 / total_mult) / integration_correction; float n_f2 = (opt_data.f2 / total_mult) / integration_correction; 
                float n_fz = (opt_data.fz / total_mult) / integration_correction; float n_f3 = (opt_data.f3 / total_mult) / integration_correction; 
                float n_f4 = (opt_data.f4 / total_mult) / integration_correction; float n_f5 = (opt_data.f5 / total_mult) / integration_correction; 
                float n_fy = (opt_data.fy / total_mult) / integration_correction; float n_fxl = (opt_data.fxl / total_mult) / integration_correction; 
                float n_f6 = (opt_data.f6 / total_mult) / integration_correction; float n_f7 = (opt_data.f7 / total_mult) / integration_correction; 
                float n_f8 = (opt_data.f8 / total_mult) / integration_correction;

                // 1. PPFD 
                float ppfd = (n_f1 + n_f2 + n_fz + n_f3 + n_f4 + n_f5 + n_fy + n_fxl + n_f6 + n_f7 + n_f8) * 0.0014f;
                payload["sensors"]["OPTICS_PPFD"] = ppfd;

                // 2. CCT (Skorygowane)
                float r_val = opt_data.f7 + opt_data.f8 + opt_data.fxl; 
                float g_val = opt_data.f4 + opt_data.f5 + opt_data.fy; 
                float b_val = opt_data.f2 + opt_data.f3 + opt_data.fz;
                float cct = 0.0f;
                if ((r_val + g_val + b_val) > 10.0f) {
                    float X = 0.4124f * r_val + 0.3576f * g_val + 0.1805f * b_val;
                    float Y = 0.2126f * r_val + 0.7152f * g_val + 0.0722f * b_val;
                    float Z = 0.0193f * r_val + 0.1192f * g_val + 0.9505f * b_val;
                    float x_chroma = X / (X + Y + Z); float y_chroma = Y / (X + Y + Z);
                    float n = (x_chroma - 0.3320f) / (0.1858f - y_chroma);
                    cct = 449.0f * pow(n, 3) + 3525.0f * pow(n, 2) + 6823.3f * n + 5520.33f;
                    payload["sensors"]["OPTICS_CCT"] = cct;
                }

                // 3. CLOUD / SKY INDEX
                float skyRatio = 0.0f;
                if ((opt_data.f8 + opt_data.nir) > 0) skyRatio = (float)(opt_data.f2 + opt_data.f3 + opt_data.fz) / (float)(opt_data.f8 + opt_data.nir);
                
                String skyStatus = "No data";
                if (opt_data.f2 < 5) skyStatus = "Night / Very Dark";
                else if (skyRatio > 1.5f) skyStatus = "Clear Blue Sky";
                else if (skyRatio > 0.9f) skyStatus = "Partly Cloudy";
                else skyStatus = "Overcast / Gray";

                payload["sensors"]["OPTICS_Sky_Ratio"] = skyRatio;
                payload["sensors"]["OPTICS_Sky_Status"] = skyStatus;

                // === 4. LOGIKA ZANIECZYSZCZENIA ŚWIETLNEGO ===
                payload["sensors"]["AS7343_Sodium_Pollution"] = opt_data.fxl; // FXL 600nm łapie doskonale światło sodowe
                String bortle = "No Data"; // Brak Danych
                if(opt_data.clear < 50) { 
                    if(opt_data.fxl > 200) bortle = "Class 7-9 (Light Smog)"; // Klasa 7-9 (Smog Świetlny)
                    else if (opt_data.fxl > 50) bortle = "Class 4-6 (Suburbs)"; // Klasa 4-6 (Przedmieścia)
                    else bortle = "Class 1-3 (Dark Sky)"; // Klasa 1-3 (Ciemne Niebo)
                } else bortle = "Daylight"; // Dzień
                payload["sensors"]["AS7343_Bortle_Class"] = bortle;
                
                ATLAS_LOG("   |- AS7343  [Spectr]: PPFD: %.2f | CCT: %.0f K | Sky: %s | Bortle: %s | Gain: %.1fX (Took: %lu ms)\n", 
                    ppfd, cct, skyStatus.c_str(), bortle.c_str(), total_mult, millis()-timer);
            }
        }
    if(veml_ok && tcaselect(MUX_VEML, CH_VEML)) {
            timer = millis();
            payload["sensors"]["VEML7700_Lux"] = veml.readLux();
            payload["sensors"]["VEML7700_White"] = veml.readWhite();
            payload["sensors"]["VEML7700_ALS_Raw"] = veml.readALS();
            ATLAS_LOG("   |- VEML7700[Optics]: %.2f Lux | White: %.2f | Raw ALS: %d (Took: %lu ms)\n", (float)payload["sensors"]["VEML7700_Lux"], (float)payload["sensors"]["VEML7700_White"], (int)payload["sensors"]["VEML7700_ALS_Raw"], millis()-timer);
        }
    if(ltr_ok && tcaselect(MUX_LTR, CH_LTR)) {
            timer = millis();
            float uvi = ltr.getUVI();
            float burn_time = calcSafeSunExposure(uvi); 
            payload["sensors"]["LTR390_UVI"] = uvi;
            payload["sensors"]["LTR390_UVS_Raw"] = ltr.getUVSData(); 
            payload["sensors"]["LTR390_Burn_Time_min"] = burn_time;
            ATLAS_LOG("   |- LTR390  [Ultrav]: UVI: %.2f | Burn Time: %.0f min | UVS Raw: %lu (Took: %lu ms)\n", 
                uvi, burn_time, (unsigned long)payload["sensors"]["LTR390_UVS_Raw"], millis()-timer);
        }

    // === VIRTUAL LUX FUSION: Combine multiple light sensors for comprehensive coverage ===
    // TSL2591 = best for day (0-188000 Lux), VEML7700 = best for night (0-120000 Lux)
    if((tsl2591_ok || veml_ok) && tcaselect(0x00, 0x00)) {
        float fused_lux = -1.0f;
        float tsl_lux = -1.0f, veml_lux = -1.0f;
        
        // Get TSL2591 reading (better for high dynamic range)
        if(tsl2591_ok && tcaselect(MUX_TSL2591, CH_TSL2591)) {
            tsl_lux = (float)payload["sensors"]["TSL2591_Lux"];
        }
        
        // Get VEML7700 reading (better for low light)
        if(veml_ok && tcaselect(MUX_VEML, CH_VEML)) {
            veml_lux = (float)payload["sensors"]["VEML7700_Lux"];
        }
        
        // Fusion logic:
        // - Night (Lux < 10): Use VEML7700 (more sensitive)
        // - Day (Lux > 10): Use weighted average or TSL2591 (wider range)
        // - Fallback: Use whichever sensor is available
        if(tsl_lux >= 0 && veml_lux >= 0) {
            // Both sensors active - use weighted fusion based on light level
            if(tsl_lux < 10) {
                fused_lux = veml_lux;  // Night: VEML more accurate
            } else if(tsl_lux > 50000) {
                fused_lux = tsl_lux;  // Bright day: TSL more accurate
            } else {
                // Twilight zone: weighted average (TSL weight increases with brightness)
                float tsl_weight = (tsl_lux / 10000.0f);  // 0-5 weight for normal range
                tsl_weight = (tsl_weight > 1.0f) ? 1.0f : tsl_weight;
                fused_lux = (veml_lux * (1.0f - tsl_weight)) + (tsl_lux * tsl_weight);
            }
        } else if(tsl_lux >= 0) {
            fused_lux = tsl_lux;
        } else if(veml_lux >= 0) {
            fused_lux = veml_lux;
        }
        
        if(fused_lux >= 0) {
            payload["sensors"]["OPTICS_Fused_Lux"] = fused_lux;
            payload["sensors"]["OPTICS_Lux_Source"] = (tsl_lux >= 0 && veml_lux >= 0) ? "Fused" : (tsl_lux >= 0 ? "TSL2591" : "VEML7700");
            ATLAS_LOG("   |- FUSION  [Lux]: Combined: %.1f Lux (TSL:%.1f VEML:%.1f) Source: %s\n", 
                fused_lux, tsl_lux, veml_lux, (const char*)payload["sensors"]["OPTICS_Lux_Source"]);
        }
    }

    if(as3935_ok && tcaselect(MUX_AS3935, CH_AS3935) && lightningTriggered) {
        uint8_t intSrc = lightning.getInterruptSrc();
        if (intSrc == 1) { 
            storm_strike_count++;
            payload["sensors"]["AS3935_Strike_Distance_Km"] = lightning.getLightningDistKm();
            payload["sensors"]["AS3935_Strike_Energy"] = lightning.getStrikeEnergyRaw();
            ATLAS_LOG("\n[!!! ALARM] RF LIGHTNING DETECTED! Distance: %d km | Energy: %lu\n\n", (int)payload["sensors"]["AS3935_Strike_Distance_Km"], (unsigned long)payload["sensors"]["AS3935_Strike_Energy"]);
        }
        lightningTriggered = false;
    }

    if(geiger_ok) {
        updateGeigerPCNT(); // ZBIERZ DANE Z MODUŁU SPRZĘTOWEGO PCNT
        unsigned long activeTimeMs = millis() - cycleStartTime;
        float minutesAwake = activeTimeMs / 60000.0f;
        float cpm = 0.0f;
        if (minutesAwake > 0) {
            cpm = (float)geiger_pulses / minutesAwake;
        }
        float uSvh = cpm / 153.8f; 
        
        payload["sensors"]["Geiger_CPM"] = cpm;
        payload["sensors"]["Geiger_uSvh"] = uSvh;
        payload["sensors"]["Geiger_Pulses_Raw"] = geiger_pulses;
        ATLAS_LOG("   |- SEN0463 [Radiat]: %.2f CPM | %.4f uSv/h | Pulses: %lu (Awake: %.1fs)\n", cpm, uSvh, geiger_pulses, activeTimeMs/1000.0f);
    }
    
    // Zawsze wysyłaj sumę burzową, aby zachować ciągłość czujnika w HA
    if (as3935_ok) {
        payload["sensors"]["AS3935_Strike_Count"] = storm_strike_count;
    }

    if(scd41_ok && tcaselect(MUX_SCD41, CH_SCD41)) {
            uint16_t co2 = 0; float temp = 0.0f, hum = 0.0f;
            scd41.readMeasurement(co2, temp, hum);
            if(co2 > 0) { 
                payload["sensors"]["SCD41_CO2_ppm"] = co2; 
                payload["sensors"]["SCD41_Temp"] = temp; 
                payload["sensors"]["SCD41_Hum"] = hum; 
            }
        }
    if(bmp585_ok && tcaselect(MUX_BMP585, CH_BMP585)) {
            if(bmp585.performReading()) {
                // Nowa biblioteka zwraca w Paskalach, dzielimy przez 100 aby mieć hPa do Twoich wzorów
                float current_press = bmp585.pressure / 100.0f; 
                float current_temp = bmp585.temperature;
                
                updateBiometeoBuffer(current_press);
                float dp_dt = current_press - last_pressure_bmp585; last_pressure_bmp585 = current_press; 
                String trend = "Stable"; // Stabilne
                if(dp_dt > 1.5) trend = "Fast Rise (Improving)"; // Szybki Wzrost (Poprawa)
                else if(dp_dt < -1.5) trend = "Fast Fall (Storm)"; // Szybki Spadek (Sztorm)
                else if(dp_dt < -0.5) trend = "Falling (Worsening)"; // Spadek (Pogorszenie)

                payload["sensors"]["BMP585_Temp"] = current_temp;
                payload["sensors"]["BMP585_Pressure_hPa"] = current_press;
                payload["sensors"]["BMP585_Trend"] = trend;
                payload["sensors"]["BMP585_dP_dt"] = dp_dt;
                float delta_3h = getPressureDelta3h(current_press);
                payload["sensors"]["METEO_Pressure_3h_Delta"] = delta_3h;

                // Track temperature history for migraine model
                float ext_t_bio = !payload["sensors"]["SHT45_Temp"].isNull()
                    ? (float)payload["sensors"]["SHT45_Temp"] : current_temp;
                updateTempHistory(ext_t_bio);
                float temp_delta_3h = getTempDelta3h(ext_t_bio);
                payload["sensors"]["METEO_Temp_3h_Delta"] = roundf(temp_delta_3h * 10.0f) / 10.0f;

                // Gather inputs
                float slp_bio  = !payload["sensors"]["METEO_Sea_Level_Press_hPa"].isNull()
                    ? (float)payload["sensors"]["METEO_Sea_Level_Press_hPa"] : current_press;
                float hum_bio  = !payload["sensors"]["SHT45_Hum"].isNull()
                    ? (float)payload["sensors"]["SHT45_Hum"] : 50.0f;
                float abs_hum  = !payload["sensors"]["METEO_Abs_Hum_g_m3"].isNull()
                    ? (float)payload["sensors"]["METEO_Abs_Hum_g_m3"] : 8.0f;
                float uvi_bio  = !payload["sensors"]["LTR390_UVI"].isNull()
                    ? (float)payload["sensors"]["LTR390_UVI"] : 0.0f;
                float voc_bio  = !payload["sensors"]["SGP41_VOC_Index"].isNull()
                    ? (float)payload["sensors"]["SGP41_VOC_Index"] : 100.0f;
                float pm25_bio = !payload["sensors"]["BMV080_PM2_5"].isNull()
                    ? (float)payload["sensors"]["BMV080_PM2_5"] : 0.0f;
                float dp_bio   = !payload["sensors"]["METEO_Dew_Point_C"].isNull()
                    ? (float)payload["sensors"]["METEO_Dew_Point_C"] : ext_t_bio - 5.0f;
                float wind_bio = wu_wind_fetched ? wu_wind_speed : 0.0f;
                float wc_bio   = calcWindChill(ext_t_bio, wind_bio);

                // ── Migraine Risk (multi-factor, v2.0) ──
                float migraine_r = calcMigraineRisk(delta_3h, dp_dt,
                    slp_bio, ext_t_bio, hum_bio, uvi_bio, voc_bio, temp_delta_3h);
                payload["sensors"]["MED_Migraine_Risk"]    = roundf(migraine_r * 10.0f) / 10.0f;
                payload["sensors"]["MED_Migraine_Cat"]     = getMigraineCategory(migraine_r);

                // ── Rheumatological / Arthritis Risk (multi-factor, v2.0) ──
                float rheum_r = calcRheumaticRisk(delta_3h, slp_bio, ext_t_bio,
                    hum_bio, abs_hum, wc_bio, dp_bio);
                payload["sensors"]["MED_Rheumatic_Risk"]   = roundf(rheum_r * 10.0f) / 10.0f;
                payload["sensors"]["MED_Rheumatic_Cat"]    = getRheumaticCategory(rheum_r);

                // ── Barometric Pain Index (Shutty 1992) ──
                float bpi = calcBarometricPainIndex(delta_3h, dp_dt, slp_bio);
                payload["sensors"]["MED_Baro_Pain_Index"]  = roundf(bpi * 10.0f) / 10.0f;

                // ── Sinus Congestion Risk ──
                float sinus_r = calcSinusRisk(delta_3h, hum_bio, ext_t_bio, pm25_bio);
                payload["sensors"]["MED_Sinus_Risk"]       = roundf(sinus_r * 10.0f) / 10.0f;

                // ── Overall Biometeo Sensitivity Score ──
                float biometeo = calcBiometeoScore(migraine_r, rheum_r, bpi, sinus_r);
                payload["sensors"]["MED_Biometeo_Score"]   = roundf(biometeo * 10.0f) / 10.0f;
                payload["sensors"]["MED_Biometeo_Alert"]   = biometeo > 7.0f ? "HIGH" : biometeo > 4.5f ? "MODERATE" : "LOW";

                ATLAS_LOG("   |- BIOMETEO: Migr:%.1f(%s) Rheum:%.1f(%s) BPI:%.1f Sinus:%.1f Score:%.1f\n",
                    migraine_r, getMigraineCategory(migraine_r).c_str(),
                    rheum_r, getRheumaticCategory(rheum_r).c_str(),
                    bpi, sinus_r, biometeo);
            }
        }
    if(ilps_ok && tcaselect(MUX_ILPS, CH_ILPS)) {
            float pressure_hPa = 0;
            float temperature_C = 0;
            ilps.GetPressure(&pressure_hPa);
            ilps.GetTemperature(&temperature_C);
            
            payload["sensors"]["ILPS22QS_Press_hPa"] = pressure_hPa;
            payload["sensors"]["ILPS22QS_Temp_C"] = temperature_C;

            // --- RĘCZNY ODCZYT DANYCH ELEKTROSTATYCZNYCH (Bypass I2C) ---
            if (tcaselect(MUX_ILPS, CH_ILPS)) {
                Wire.beginTransmission(0x5C); 
                Wire.write(0x2D); // Rejestr QVAR_OUT_L (Młodszy bajt ładunku)
                if (Wire.endTransmission(false) == 0) {
                    Wire.requestFrom((uint16_t)0x5C, (uint8_t)2);
                    if (Wire.available() == 2) {
                        uint8_t qvar_l = Wire.read();
                        uint8_t qvar_h = Wire.read();
                        
                        // Złożenie dwóch bajtów w jedną 16-bitową liczbę
                        int16_t qvar_raw = (qvar_h << 8) | qvar_l; 
                        
                        // Konwersja do czytelnego formatu 
                        // (Wartość surowa * mnożnik w zależności od impedancji Twojej anteny)
                        float qvar_mv = (float)qvar_raw * 0.001f; 
                        payload["sensors"]["ILPS22QS_QVAR_mV"] = qvar_mv;
                    }
                }
            }
        }

    if(as7331_ok && as7331.readAllUV() == 0) { 
        float uv_a = as7331.getUVA();
        float uv_b = as7331.getUVB();
        float uv_c = as7331.getUVC();
        payload["sensors"]["AS7331_UVA"] = uv_a; 
        payload["sensors"]["AS7331_UVB"] = uv_b;
        payload["sensors"]["AS7331_UVC"] = uv_c;
        payload["sensors"]["AS7331_Medical_UVI"] = (uv_a * 0.001f) + (uv_b * 1.2f);
        
        ATLAS_LOG("   |- AS7331  [Med UV]: UVA:%.2f UVB:%.2f UVC:%.2f | UVI:%.2f (Took: %lu ms)\n", uv_a, uv_b, uv_c, (float)payload["sensors"]["AS7331_Medical_UVI"], 0UL);
    }

    // --- AI & METEO MATH FUSION (CALCULATED ENTITIES) ---
    if (!payload["sensors"]["BMP585_Pressure_hPa"].isNull() && !payload["sensors"]["SHT45_Temp"].isNull()) {
        float p = payload["sensors"]["BMP585_Pressure_hPa"]; float t = payload["sensors"]["SHT45_Temp"];
        float h = !payload["sensors"]["SHT45_Hum"].isNull() ? (float)payload["sensors"]["SHT45_Hum"] : 50.0f;
        payload["sensors"]["METEO_Density_Altitude_m"] = calcDensityAltitude(p, t);
        payload["sensors"]["METEO_VPD_kPa"] = calcVPD(t, h);
        payload["sensors"]["METEO_Boiling_Point_C"] = calcBoilingPoint(p);
        payload["sensors"]["AVIATION_TAS_Proxy_kt"] = calcTASProxy(p, t);
        payload["sensors"]["AVIATION_Icing_Risk"] = calcIcingRisk(t, h);
        payload["sensors"]["AVIATION_QFE_hPa"] = calcQFE(p);
        
        // Advanced v2.2 Formulas
        float slp = calcSLP(p, t, STATION_ALTITUDE_METERS);
        float d3h = getPressureDelta3h(p);
        time_t now_calc = time(nullptr);
        struct tm* tm_calc = localtime(&now_calc);
        payload["sensors"]["METEO_Zambretti_Forecast"] = calcZambretti(slp, d3h, tm_calc->tm_mon + 1);
        payload["sensors"]["METEO_Frost_Risk_Pct"] = calcFrostRisk(t, calcDewPoint(t, h), tm_calc->tm_hour);
        payload["sensors"]["METEO_Speed_Of_Sound_ms"] = calcSpeedOfSound(t, h, p);
        payload["sensors"]["METEO_Mixing_Ratio_gkg"] = calcMixingRatio(t, h, p);

        float vpd = (float)payload["sensors"]["METEO_VPD_kPa"];
        payload["sensors"]["ENV_Fire_Risk_Pct"] = calcFireRisk(t, h, vpd);
        payload["sensors"]["MED_Stroke_Risk"] = calcStrokeRisk(d3h);
        
        if (!payload["sensors"]["SGP41_VOC_Index"].isNull() && !payload["sensors"]["BMV080_PM2_5"].isNull()) {
            payload["sensors"]["MED_Asthma_Risk"] = calcAsthmaRisk((float)payload["sensors"]["BMV080_PM2_5"], h, t, (float)payload["sensors"]["SGP41_VOC_Index"]);
        }

        // --- GAS IDENTIFICATION SYSTEM ---
        float ge1 = !payload["sensors"]["BME688_Gas_Est_1"].isNull() ? (float)payload["sensors"]["BME688_Gas_Est_1"] : 0.0f;
        float ge2 = !payload["sensors"]["BME688_Gas_Est_2"].isNull() ? (float)payload["sensors"]["BME688_Gas_Est_2"] : 0.0f;
        float ge3 = !payload["sensors"]["BME688_Gas_Est_3"].isNull() ? (float)payload["sensors"]["BME688_Gas_Est_3"] : 0.0f;
        float ge4 = !payload["sensors"]["BME688_Gas_Est_4"].isNull() ? (float)payload["sensors"]["BME688_Gas_Est_4"] : 0.0f;
        float voc = !payload["sensors"]["SGP41_VOC_Index"].isNull() ? (float)payload["sensors"]["SGP41_VOC_Index"] : -1.0f;
        float nox = !payload["sensors"]["SGP41_NOx_Index"].isNull() ? (float)payload["sensors"]["SGP41_NOx_Index"] : -1.0f;
        float co2 = !payload["sensors"]["SCD41_CO2_ppm"].isNull() ? (float)payload["sensors"]["SCD41_CO2_ppm"] : (!payload["sensors"]["BME688_eCO2"].isNull() ? (float)payload["sensors"]["BME688_eCO2"] : -1.0f);
        float iaq = !payload["sensors"]["BME688_IAQ"].isNull() ? (float)payload["sensors"]["BME688_IAQ"] : -1.0f;
        float pm25 = !payload["sensors"]["BMV080_PM2_5"].isNull() ? (float)payload["sensors"]["BMV080_PM2_5"] : 0.0f;
        
        if (ge1 > 0 || ge2 > 0 || voc >= 0) {
            // Gas pattern identification
            if (voc >= 0 && co2 >= 0 && iaq >= 0) {
                payload["sensors"]["GAS_Identified_Pattern"] = identifyGasPattern(ge1, ge2, ge3, ge4, voc, nox, co2, iaq);
            }
            // Gas type category
            if (ge1 > 0 || ge2 > 0 || ge3 > 0 || ge4 > 0) {
                payload["sensors"]["GAS_Type_Category"] = getGasTypeCategory(ge1, ge2, ge3, ge4);
            }
            // Toxicity risk
            if (voc >= 0 && nox >= 0 && co2 >= 0 && iaq >= 0) {
                int tox_risk = calcToxicityRisk(voc, nox, co2, iaq);
                payload["sensors"]["GAS_Toxicity_Risk"] = tox_risk;
                payload["sensors"]["GAS_Toxicity_Name"] = getToxicityName(tox_risk);
            }
            // Indoor air quality score
            if (iaq >= 0 && voc >= 0 && co2 >= 0) {
                payload["sensors"]["GAS_IAQ_Score"] = calcIndoorAirQualityScore(iaq, voc, co2, pm25);
            }
        }

        if (!payload["sensors"]["Geiger_CPM"].isNull()) {
            payload["sensors"]["Geiger_CPM_Corrected"] = calcCorrectedGCR((float)payload["sensors"]["Geiger_CPM"], p);
        }
    }

    if (!payload["sensors"]["BMV080_PM2_5"].isNull()) {
        float pm25 = payload["sensors"]["BMV080_PM2_5"];
        float pm10 = payload["sensors"]["BMV080_PM10_0"];
        float hum = !payload["sensors"]["SHT45_Hum"].isNull() ? (float)payload["sensors"]["SHT45_Hum"] : 50.0f;
        float nox = !payload["sensors"]["SGP41_NOx_Index"].isNull() ? (float)payload["sensors"]["SGP41_NOx_Index"] : 1.0f;
        
        int eaqi = calcEAQI(pm25, pm10);
        float who_aqi = calcWHO_AQI(pm25, pm10);
        String air_warning = getAQIWarningLevel(eaqi);
        
        payload["sensors"]["AIR_EAQI_Index"] = eaqi;
        payload["sensors"]["AIR_WHO_AQI_Pct"] = who_aqi;
        payload["sensors"]["AIR_Quality_Status"] = air_warning;
        payload["sensors"]["AIR_Smog_Index"] = calcSmogIndex(pm25, hum, nox);
        payload["sensors"]["AIR_Visibility_Km"] = calcVisibility(pm25, pm10, hum);
        
        // Warning Logic
        if (eaqi >= 4) payload["alerts"]["Air_Quality"] = "HAZARDOUS";
        else if (eaqi >= 3) payload["alerts"]["Air_Quality"] = "MODERATE";

        // ── Pulmonary Deposition Estimate — D = C_PM2.5 × Ve × t ─────────────
        // Formula from image: D = C_PM2.5 × Ve × t  (µg inhaled per minute)
        // Ve = 12 L/min (adult normal tidal volume × resp. rate)
        //    = 25 L/min (moderate exercise),   45 L/min (heavy exercise)
        // Deposited fraction ≈ 30% of PM2.5 reaches alveoli (ICRP 1994 model)
        // Result: µg/min deposited in lungs at rest
        {
            const float Ve_rest_Lmin = 12.0f;     // liters per minute at rest
            const float deposit_frac = 0.30f;     // 30% alveolar deposition (ICRP)
            const float Ve_m3min = Ve_rest_Lmin / 1000.0f;  // → m³/min
            float depo_rest  = pm25 * 1e-6f * Ve_m3min * deposit_frac * 1e6f;  // µg/min
            float depo_exer  = pm25 * 1e-6f * (25.0f/1000.0f) * deposit_frac * 1e6f; // exercise µg/min

            payload["sensors"]["MED_Lung_Deposit_Rest"]    = roundf(depo_rest  * 1000.0f) / 1000.0f;  // µg/min
            payload["sensors"]["MED_Lung_Deposit_Exer"]   = roundf(depo_exer  * 1000.0f) / 1000.0f;  // µg/min
        }

        // ── BME688 Smoke Type Classifier ──────────────────────────────────────
        // Classifies smoke based on MOX resistance profile in combination with
        // PM concentration and VOC/NOx ratio from SGP41.
        // Wood smoke:    high PM, moderate VOC, low NOx, low-mid gas Resistance
        // Plastic smoke: high PM, very high VOC, elevated NOx, low gas Resistance
        // Cooking:       moderate PM, high VOC, low NOx, high gas resistance drop
        // Tobacco:       moderate PM, high bVOC (aldehydes), moderate NOx
        // Dust:          elevated PM with no VOC/NOx increase
        // Clean air:     all low
        if (!payload["sensors"]["BME688_Gas_Res_Raw"].isNull()) {
            float gas_r = payload["sensors"]["BME688_Gas_Res_Raw"];
            float bvoc  = !payload["sensors"]["BME688_bVOC"].isNull()
                          ? (float)payload["sensors"]["BME688_bVOC"] : 0.0f;
            float nox   = !payload["sensors"]["SGP41_NOx_Index"].isNull()
                          ? (float)payload["sensors"]["SGP41_NOx_Index"] : 1.0f;
            float voc   = !payload["sensors"]["SGP41_VOC_Index"].isNull()
                          ? (float)payload["sensors"]["SGP41_VOC_Index"] : 100.0f;

            String smoke_type = "Clean Air";
            if (pm25 > 15.0f) {  // threshold: detectable smoke
                if (gas_r < 5000.0f && voc > 200.0f && nox > 15.0f) {
                    smoke_type = "Plastic/Synthetic Combustion";
                } else if (gas_r < 10000.0f && voc > 150.0f && nox < 10.0f) {
                    smoke_type = "Wood/Biomass Smoke";
                } else if (bvoc > 1.5f && voc > 200.0f && nox < 5.0f) {
                    smoke_type = "Tobacco / Cigarette";
                } else if (voc > 250.0f && pm25 < 30.0f) {
                    smoke_type = "Cooking Fumes";
                } else if (pm25 > 30.0f && voc < 120.0f && nox < 5.0f) {
                    smoke_type = "Dust / Mineral Particles";
                } else {
                    smoke_type = "Mixed / Unclassified";
                }
            }
            payload["sensors"]["GAS_Smoke_Type"] = smoke_type;
        }
    }

    // ── SOLAR RADIATION + AEROSOL OPTICAL DEPTH ─────────────────────────────
    {
        float lux = !payload["sensors"]["TSL2591_Lux"].isNull() ? (float)payload["sensors"]["TSL2591_Lux"] : 0.0f;
        float uvi = !payload["sensors"]["LTR390_UVI"].isNull()  ? (float)payload["sensors"]["LTR390_UVI"]  : 0.0f;
        float slp = !payload["sensors"]["METEO_Sea_Level_Press_hPa"].isNull()
                    ? (float)payload["sensors"]["METEO_Sea_Level_Press_hPa"] : 1013.25f;

        float ghi = calcGHI_Wm2(lux, uvi);
        payload["sensors"]["METEO_Solar_GHI_Wm2"] = roundf(ghi * 10.0f) / 10.0f;

        float aod = calcAOD_proxy(ghi, uvi, slp);
        if (aod >= 0.0f) payload["sensors"]["ASTRO_AOD"] = roundf(aod * 1000.0f) / 1000.0f;

        // CO2 ventilation requirement
        float co2_vent = !payload["sensors"]["SCD41_CO2_ppm"].isNull()
                         ? (float)payload["sensors"]["SCD41_CO2_ppm"]
                         : (!payload["sensors"]["BME688_eCO2"].isNull() ? (float)payload["sensors"]["BME688_eCO2"] : -1.0f);
        if (co2_vent > 0) payload["sensors"]["MED_Required_ACH"] = roundf(calcRequiredACH(co2_vent) * 10.0f) / 10.0f;

        ATLAS_LOG("   |- SOLAR  [Radiation]: GHI:%.1f W/m² | AOD:%.3f | CO2→ACH:%.1f/h (Took: 0 ms)\n",
            ghi, (aod >= 0 ? aod : -1.0f), co2_vent > 0 ? calcRequiredACH(co2_vent) : 0.0f);
    }

    // ── OZONE PROXY (AS7331 UVB/UVA ratio) ──────────────────────────────────
    if (!payload["sensors"]["AS7331_UVA"].isNull() && !payload["sensors"]["AS7331_UVB"].isNull()) {
        float uva  = payload["sensors"]["AS7331_UVA"];
        float uvb  = payload["sensors"]["AS7331_UVB"];
        float uvi2 = !payload["sensors"]["LTR390_UVI"].isNull() ? (float)payload["sensors"]["LTR390_UVI"] : 0.0f;
        float ozone_du = calcOzoneProxy_DU(uvb, uva, uvi2);
        if (ozone_du > 0) {
            payload["sensors"]["SPACE_Ozone_DU"] = roundf(ozone_du);
            ATLAS_LOG("   |- OZONE  [AS7331]: UVA:%.2f UVB:%.2f → Ozone proxy: %.0f DU\n", uva, uvb, ozone_du);
        }
    }

    // ── GEOMAGNETIC K-INDEX PROXY + AURORA + dB/dt SSC ──────────────────────
    if (!payload["sensors"]["BMM350_Mag_X"].isNull()) {
        float bx = payload["sensors"]["BMM350_Mag_X"];
        float by = payload["sensors"]["BMM350_Mag_Y"];
        float bz = payload["sensors"]["BMM350_Mag_Z"];
        float btot = sqrtf(bx*bx + by*by + bz*bz); // µT
        updateMagBaseline(btot);
        int k_proxy = calcKIndexProxy(btot);
        payload["sensors"]["SPACE_K_Index_Proxy"]  = k_proxy;
        payload["sensors"]["SPACE_B_Total_uT"]     = roundf(btot * 100.0f) / 100.0f;
        payload["sensors"]["SPACE_B_Baseline_uT"]  = roundf(mag_baseline_val * 100.0f) / 100.0f;
        payload["sensors"]["SPACE_B_Dev_nT"]       = roundf((btot - mag_baseline_val) * 1000.0f);

        float aurora_prob = calcAuroraProbability(k_proxy, STATION_GEOMAG_LAT);
        payload["sensors"]["SPACE_Aurora_Prob_Pct"] = roundf(aurora_prob * 10.0f) / 10.0f;
        payload["sensors"]["SPACE_Aurora_Alert"]    = (aurora_prob > 5.0f) ? "YES" : "No";

        // dB/dt — CME Sudden Storm Commencement detector
        float dbdt = calcDBdt_nTs(bx, by, bz);
        payload["sensors"]["SPACE_dBdt_nTs"]       = roundf(dbdt * 100.0f) / 100.0f;
        payload["sensors"]["SPACE_SSC_Class"]      = classifySSC(dbdt);
        if (dbdt > 5.0f) {
            payload["alerts"]["Geomagnetic"] = "SSC DETECTED";
        }

        ATLAS_LOG("   |- GEOPHY [K/SSC]: |B|:%.2fuT Dev:%.0fnT K≈%d dB/dt:%.2fnT/s(%s) Aurora:%.1f%%\n",
            btot, (btot - mag_baseline_val)*1000.0f, k_proxy, dbdt, classifySSC(dbdt).c_str(), aurora_prob);
    }

    // ── FORBUSH DECREASE — GCR 7-day baseline deviation ──────────────────────
    if (!payload["sensors"]["Geiger_CPM_Corrected"].isNull()) {
        float gcr = payload["sensors"]["Geiger_CPM_Corrected"];
        updateGCRHistory(gcr);
        float forbush = calcForbushDecrease(gcr);
        payload["sensors"]["SPACE_Forbush_Pct"]  = roundf(forbush * 10.0f) / 10.0f;
        payload["sensors"]["SPACE_Forbush_Class"] = classifyForbush(forbush);
        if (forbush < -5.0f) payload["alerts"]["CosmicRay"] = "FORBUSH DECREASE";
        ATLAS_LOG("   |- GCR    [Forbush]: CPM_corr:%.1f Delta:%.1f%% (%s)\n",
            gcr, forbush, classifyForbush(forbush).c_str());
    }

    // ── SEISMIC PROXY (ILPS22QS QVAR) ────────────────────────────────────────
    if (!payload["sensors"]["ILPS_QVAR"].isNull()) {
        float qvar = payload["sensors"]["ILPS_QVAR"];
        float seismic = calcSeismicProxy(qvar);
        payload["sensors"]["GEOPHY_Seismic_Proxy"] = roundf(seismic * 10.0f) / 10.0f;
        if (seismic > 6.0f) payload["alerts"]["Seismic"] = "ELEVATED";
    }

    if (!payload["sensors"]["AS7343_FZ_450nm"].isNull() && !payload["sensors"]["AS7343_Clear"].isNull()) {
        float f3_475 = payload["sensors"]["AS7343_F3_475nm"]; float f_clear = payload["sensors"]["AS7343_Clear"];
        String melatonin = "Blocked (Daylight)"; // Zablokowane (Dzień)
        if (f_clear < 150 && f3_475 < 20) melatonin = "Initiated (Dusk)"; // Rozpoczęte (Szarówka)
        if (f_clear < 20 && f3_475 < 5) melatonin = "Full Secretion (Bio Night)"; // Pełne Wydzielanie (Noc Biologiczna)
        payload["sensors"]["OPTICS_Melatonin_Status"] = melatonin;
    }

    if(rg15_ok) {
        Serial1.print("r\n"); // Request data
        unsigned long start = millis();
        while (millis() - start < 500) {
            if (Serial1.available()) {
                String resp = Serial1.readStringUntil('\n');
                if (resp.indexOf("Acc") >= 0) {
                    // Example format: Acc  0.00 mm, EventAcc  0.00 mm, TotalAcc  0.00 mm, RInt  0.00 mmph
                    int accPos = resp.indexOf("Acc");
                    int eventPos = resp.indexOf("EventAcc");
                    int totalPos = resp.indexOf("TotalAcc");
                    int intPos = resp.indexOf("RInt");
                    
                    if (accPos >= 0 && eventPos >= 0) {
                        payload["sensors"]["RG15_Acc_mm"] = resp.substring(accPos + 4, eventPos).toFloat();
                        payload["sensors"]["RG15_EventAcc_mm"] = resp.substring(eventPos + 9, totalPos).toFloat();
                        payload["sensors"]["RG15_TotalAcc_mm"] = resp.substring(totalPos + 9, intPos).toFloat();
                        payload["sensors"]["RG15_Intensity_mmph"] = resp.substring(intPos + 5).toFloat();
                        payload["sensors"]["RG15_Daily_Rain_mm"] = payload["sensors"]["RG15_EventAcc_mm"]; // Mapping for API compatibility
                    }
                    break;
                }
            }
        }
    }

    String modeName = "Continuous";
    if (currentMode == MODE_LIGHT_SLEEP) modeName = "Light Sleep";
    else if (currentMode == MODE_DEEP_SLEEP) modeName = "Deep Sleep";
    
    payload["sensors"]["System_Mode"] = modeName;
    payload["sensors"]["Fault_BME280"] = bme280_ok ? "OFF" : "ON";
    payload["sensors"]["Fault_BMV080"] = bmv080_ok ? "OFF" : "ON";
    payload["sensors"]["Fault_INA219"] = ina219_ok ? "OFF" : "ON";
    payload["sensors"]["Fault_MS8607"] = ms8607_ok ? "OFF" : "ON";
    payload["sensors"]["Fault_MAX17048"] = max17048_ok ? "OFF" : "ON";
    payload["sensors"]["Fault_BME690"]    = bme688_ok    ? "OFF" : "ON"; // BME690 = bme688 object
    payload["sensors"]["Fault_SHT45"]    = sht45_ok     ? "OFF" : "ON";
    payload["sensors"]["Fault_SGP41"]    = sgp41_ok     ? "OFF" : "ON";
    payload["sensors"]["Fault_SCD41"]    = scd41_ok     ? "OFF" : "ON"; // was missing!
    payload["sensors"]["Fault_BMP585"]   = bmp585_ok    ? "OFF" : "ON"; // was missing!
    payload["sensors"]["Fault_ILPS22QS"] = ilps_ok      ? "OFF" : "ON"; // was missing!
    payload["sensors"]["Fault_TSL2591"]  = tsl2591_ok   ? "OFF" : "ON";
    payload["sensors"]["Fault_OPT4048"]  = opt4048_ok   ? "OFF" : "ON";
    payload["sensors"]["Fault_TCS34725"] = tcs34725_ok  ? "OFF" : "ON";
    payload["sensors"]["Fault_BMM350"]   = bmm350_ok    ? "OFF" : "ON";
    payload["sensors"]["Fault_LSM6DSOX"] = lsm6dsox_ok  ? "OFF" : "ON";
    payload["sensors"]["Fault_LIS3MDL"]  = lis3mdl_ok   ? "OFF" : "ON";
    payload["sensors"]["Fault_I2CMemory"]= i2c_mem_ok   ? "OFF" : "ON";
    payload["sensors"]["Fault_AS7343"]   = as7343_ok    ? "OFF" : "ON";
    payload["sensors"]["Fault_AS7331"]   = as7331_ok    ? "OFF" : "ON"; // was missing!
    payload["sensors"]["Fault_VEML7700"] = veml_ok      ? "OFF" : "ON";
    payload["sensors"]["Fault_LTR390"]   = ltr_ok       ? "OFF" : "ON";
    payload["sensors"]["Fault_AS3935"]   = as3935_ok    ? "OFF" : "ON";
    payload["sensors"]["Fault_Geiger"]   = geiger_ok    ? "OFF" : "ON";
    payload["sensors"]["Fault_RG15"]     = rg15_ok      ? "OFF" : "ON";
    payload["sensors"]["Fault_MLX90640"] = mlx90640_ok  ? "OFF" : "ON"; // was missing!
    payload["sensors"]["Fault_ZMOD4510"] = zmod4510_ok  ? "OFF" : "ON"; // was missing!

    // Compatibility warnings (non-blocking)
    payload["sensors"]["Warn_AS7343_Compat"] = (!payload["sensors"]["AS7343_Chip_ID"].isNull() && (int)payload["sensors"]["AS7343_Chip_ID"] == 0x09) ? "AS7341 detected" : "OFF";
    
    // --- ANOMALY DETECTION PROCESSING ---
    float c_t = !payload["sensors"]["SHT45_Temp"].isNull() ? (float)payload["sensors"]["SHT45_Temp"] : 
                (!payload["sensors"]["ENV_Ext_Temp"].isNull() ? (float)payload["sensors"]["ENV_Ext_Temp"] : -999.0f);
    float c_h = !payload["sensors"]["SHT45_Hum"].isNull() ? (float)payload["sensors"]["SHT45_Hum"] : 
                (!payload["sensors"]["ENV_Ext_Hum"].isNull() ? (float)payload["sensors"]["ENV_Ext_Hum"] : -999.0f);
    float c_pm = !payload["sensors"]["BMV080_PM2_5"].isNull() ? (float)payload["sensors"]["BMV080_PM2_5"] : -999.0f;
    float c_voc = !payload["sensors"]["SGP41_VOC_Index"].isNull() ? (float)payload["sensors"]["SGP41_VOC_Index"] : -999.0f;
    float c_co2 = !payload["sensors"]["SCD41_CO2_ppm"].isNull() ? (float)payload["sensors"]["SCD41_CO2_ppm"] : -999.0f;
    float c_p = !payload["sensors"]["BMP585_Pressure_hPa"].isNull() ? (float)payload["sensors"]["BMP585_Pressure_hPa"] : -999.0f;
    float c_cpm = !payload["sensors"]["Geiger_CPM"].isNull() ? (float)payload["sensors"]["Geiger_CPM"] : -999.0f;

    String a_temp = detectSingleAnomaly(c_t, anom_hist_temp, 3.0f);   // 3.0C change
    String a_hum = detectSingleAnomaly(c_h, anom_hist_hum, 15.0f);    // 15% change
    String a_pm25 = detectSingleAnomaly(c_pm, anom_hist_pm25, 25.0f); // 25ug/m3 change
    String a_voc = detectSingleAnomaly(c_voc, anom_hist_voc, 40.0f);  // 40 index points change
    String a_co2 = detectSingleAnomaly(c_co2, anom_hist_co2, 300.0f); // 300ppm change
    String a_press = detectSingleAnomaly(c_p, anom_hist_press, 4.0f); // 4 hPa change (sudden storm system)
    String a_cpm = detectSingleAnomaly(c_cpm, anom_hist_cpm, 25.0f);  // 25 CPM radiation spike

    bool is_anomaly = false;
    String anomaly_desc = "";
    if (a_temp == "ON") { is_anomaly = true; anomaly_desc += "Temperature "; }
    if (a_hum == "ON") { is_anomaly = true; anomaly_desc += "Humidity "; }
    if (a_pm25 == "ON") { is_anomaly = true; anomaly_desc += "PM2.5 "; }
    if (a_voc == "ON") { is_anomaly = true; anomaly_desc += "VOC "; }
    if (a_co2 == "ON") { is_anomaly = true; anomaly_desc += "CO2 "; }
    if (a_press == "ON") { is_anomaly = true; anomaly_desc += "AirPressure "; }
    if (a_cpm == "ON") { is_anomaly = true; anomaly_desc += "Radiation "; }

    payload["system"]["anomaly_active"] = is_anomaly;
    if (is_anomaly) {
        payload["system"]["anomaly_desc"] = anomaly_desc;
        ATLAS_LOG("\n[!!! ANOMALY WARNING !!!] Sudden environmental shift: %s\n", anomaly_desc.c_str());
    }

    updateAnomalyBuffers(c_t, c_h, c_pm, c_voc, c_co2, c_p, c_cpm);

    // --- VIRTUAL DIAGNOSTIC SENSORS ---
    int current_health_score = 0;
    if(bme280_ok) current_health_score++; if(bmv080_ok) current_health_score++;
    if(ina219_ok) current_health_score++; if(ms8607_ok) current_health_score++;
    if(max17048_ok) current_health_score++; if(bme688_ok) current_health_score++;
    if(sht45_ok) current_health_score++; if(sgp41_ok) current_health_score++;
    if(opt4048_ok) current_health_score++; if(tcs34725_ok) current_health_score++;
    if(tsl2591_ok) current_health_score++; if(i2c_mem_ok) current_health_score++;
    if(as7343_ok) current_health_score++; if(veml_ok) current_health_score++;
    if(ltr_ok) current_health_score++; if(as3935_ok) current_health_score++;
    if(geiger_ok) current_health_score++; if(scd41_ok) current_health_score++;
    if(bmp585_ok) current_health_score++; if(ilps_ok) current_health_score++;
    if(as7331_ok) current_health_score++; if(rg15_ok) current_health_score++;

    // Sprawdzamy czy była zmiana zdrowia czujników i aktualizujemy czas utraty/odzyskania
    time_t now_ts = time(nullptr);
    if (now_ts > 1600000000) { // Jeżeli NTP jest zsynchronizowane
        if (cold_boot_timestamp == 0) {
            // Próba odzyskania z NVS po twardym resecie
            prefs.begin("diag", true);
            cold_boot_timestamp = prefs.getULong("boot_ts", 0);
            last_fault_timestamp = prefs.getULong("fault_ts", 0);
            prefs.end();
            
            if (cold_boot_timestamp == 0) {
                cold_boot_timestamp = now_ts; // Złapanie pierwszego momentu synchronizacji czasu po pełnym restarcie
                prefs.begin("diag", false);
                prefs.putULong("boot_ts", cold_boot_timestamp);
                prefs.end();
            }
        }
        
        payload["sensors"]["System_Uptime_Hours"] = (float)(now_ts - cold_boot_timestamp) / 3600.0f;

        if (previous_sensor_health_score != -1 && current_health_score != previous_sensor_health_score) {
            last_fault_timestamp = now_ts;
            prefs.begin("diag", false);
            prefs.putULong("fault_ts", last_fault_timestamp);
            prefs.end();
        }
        previous_sensor_health_score = current_health_score;

        if (last_fault_timestamp > 0) {
            payload["sensors"]["System_Last_Fault_Hours_Ago"] = (float)(now_ts - last_fault_timestamp) / 3600.0f;
        }
    }

    // Aktualizacja bufora dla Sensor LoadAvg
    sensor_health_hist[health_hist_idx] = current_health_score;
    health_hist_idx++;
    if (health_hist_idx >= HEALTH_HIST_SIZE) {
        health_hist_idx = 0;
        health_hist_filled = true;
    }

    float load_5 = 0, load_10 = 0, load_15 = 0;
    int limit_5 = min(5, health_hist_filled ? HEALTH_HIST_SIZE : health_hist_idx);
    int limit_10 = min(10, health_hist_filled ? HEALTH_HIST_SIZE : health_hist_idx);
    int limit_15 = health_hist_filled ? HEALTH_HIST_SIZE : health_hist_idx;

    if (limit_15 > 0) {
        int sum_5 = 0, sum_10 = 0, sum_15 = 0;
        for (int i = 0; i < limit_15; i++) {
            int idx = (health_hist_idx - 1 - i + HEALTH_HIST_SIZE) % HEALTH_HIST_SIZE;
            int val = sensor_health_hist[idx];
            if (i < 5) sum_5 += val;
            if (i < 10) sum_10 += val;
            sum_15 += val;
        }
        load_5 = (float)sum_5 / limit_5;
        load_10 = (float)sum_10 / limit_10;
        load_15 = (float)sum_15 / limit_15;
        
        payload["sensors"]["System_Health_LoadAvg_5"] = load_5;
        payload["sensors"]["System_Health_LoadAvg_10"] = load_10;
        payload["sensors"]["System_Health_LoadAvg_15"] = load_15;
    }
}

void sleepSensors(bool deepSleep) {
    ATLAS_LOG("\n[POWER MANAGER] Executing sensor sleep sequence...\n");
    if(bme280_ok && tcaselect(MUX_BME280, CH_BME280)) bme280.setSampling(Adafruit_BME280::MODE_SLEEP);
    if(max17048_ok && tcaselect(MUX_MAX17048, CH_MAX17048)) bat.sleep(true);
    if(tsl2591_ok && tcaselect(MUX_TSL2591, CH_TSL2591)) tsl2591.disable();
    if(tcs34725_ok && tcaselect(MUX_TCS34725, CH_TCS34725)) tcs34725.disable();


    if(tcaselect(MUX_VEML, CH_VEML)) {
        if(veml_ok) veml.powerSaveEnable(true);
        // AS7343 od SparkFun przechodzi w tryb Standby automatycznie między pomiarami
    }
	
    if(scd41_ok && tcaselect(MUX_SCD41, CH_SCD41)) scd41.powerDown();
   
    if (deepSleep) {
        if(bmv080_ok && tcaselect(MUX_BMV080, CH_BMV080)) {
            Wire.beginTransmission(0x57);
            if(Wire.endTransmission() == 0) bmv080.setMode(0); 
        }
        // W trybie Deep Sleep wylaczamy sprzet (brak detachInterrupt poniewaz zostalo zastapione przez sprzetowy PCNT)
    }
    
	static unsigned long lastNvsSave = 0;
    if(bme688_ok && tcaselect(MUX_BME688, CH_BME688) && (millis() - lastNvsSave > bme688Config.stateSavePeriodMs || deepSleep)) { 
        uint8_t state[BSEC_MAX_STATE_BLOB_SIZE];
        if (envSensor.getState(state)) {
            // Zapis do EEPROM
            if (i2c_mem_ok) {
                writeExtEEPROM(0x0010, state, BSEC_MAX_STATE_BLOB_SIZE);
                ATLAS_LOG("[INFO] BME688 AI State committed to EXTERNAL EEPROM.\n");
            }
			// tego nie lubimy, ale jesli eeprom sie wyjebal to problem
            // Zapis do NVS Flash (Backup)
            prefs.begin("bsec_data", false);
            prefs.putBytes("bsec_state", state, BSEC_MAX_STATE_BLOB_SIZE);
            prefs.end();
            if (!i2c_mem_ok) ATLAS_LOG("[INFO] BME688 AI State committed to internal NVS Flash (EEPROM Offline).\n");
            
            lastNvsSave = millis();
        }
    }
    
    ATLAS_LOG("[POWER MANAGER] Hardware gracefully put to sleep.\n");
}

// [TODO_HW#0001]
// HMMM ...
// ROZWAZYC CZY NIE LEPIEJ ZASTOSOWAC MODUL NA KARTY SD i WYDELEGOWAC TO na FAT32
// ---------------------------------------------------------------
// 5. WEB SERVER, UI & OTA
// ---------------------------------------------------------------

const char index_html[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8"><meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
<title>ATLAS-OS v2.3</title>
<link rel="preconnect" href="https://fonts.googleapis.com">
<link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>
<link href="https://fonts.googleapis.com/css2?family=Inter:wght@300;400;500;600&family=JetBrains+Mono:wght@400;500&display=swap" rel="stylesheet">
<style>
    :root {
        --bg:#0a0a12;--glass:rgba(28,28,44,0.72);--glass-border:rgba(255,255,255,0.08);
        --accent:#00d4aa;--accent-dim:#00a080;--danger:#ff4757;--warning:#ffa502;
        --text:#e8e8f0;--text-dim:#8888a0;--text-muted:#555566;
        --chart-h:80px;
    }
    *{box-sizing:border-box;-webkit-tap-highlight-color:transparent;margin:0;padding:0}
    body{background:var(--bg);background-image:
        radial-gradient(ellipse at 15% 15%,rgba(0,212,170,.10) 0%,transparent 50%),
        radial-gradient(ellipse at 85% 85%,rgba(100,80,200,.08) 0%,transparent 50%);
        color:var(--text);font-family:'Inter',-apple-system,sans-serif;
        overflow:hidden;height:100vh}
    /* ── Desktop ── */
    #desktop{position:relative;width:100%;height:calc(100vh - 56px);padding:14px;
        overflow-y:auto;overflow-x:hidden;-webkit-overflow-scrolling:touch}
    /* ── Dock ── */
    .dock{position:fixed;bottom:10px;left:50%;transform:translateX(-50%);
        height:50px;background:rgba(18,18,30,.88);backdrop-filter:blur(24px);
        -webkit-backdrop-filter:blur(24px);border-radius:16px;
        border:1px solid var(--glass-border);display:flex;align-items:center;
        padding:0 8px;gap:3px;z-index:9999;
        box-shadow:0 8px 32px rgba(0,0,0,.45),inset 0 1px 0 rgba(255,255,255,.05)}
    .dock-item{width:42px;height:42px;border-radius:11px;display:flex;align-items:center;
        justify-content:center;cursor:pointer;font-size:20px;color:var(--text-dim);
        transition:all .18s cubic-bezier(.34,1.56,.64,1)}
    .dock-item:hover{transform:translateY(-5px) scale(1.1);color:var(--text);background:rgba(255,255,255,.07)}
    .dock-item.active{color:var(--accent);background:rgba(0,212,170,.15)}
    /* ── Glass Window ── */
    .window{position:absolute;background:var(--glass);backdrop-filter:blur(28px) saturate(170%);
        -webkit-backdrop-filter:blur(28px) saturate(170%);border:1px solid var(--glass-border);
        border-radius:14px;box-shadow:0 14px 44px rgba(0,0,0,.4),inset 0 1px 0 rgba(255,255,255,.06);
        display:flex;flex-direction:column;overflow:hidden;
        animation:winIn .3s cubic-bezier(.34,1.56,.64,1)}
    @keyframes winIn{from{opacity:0;transform:scale(.93) translateY(8px)}to{opacity:1;transform:scale(1) translateY(0)}}
    .win-header{background:rgba(255,255,255,.03);padding:11px 14px;cursor:grab;
        display:flex;justify-content:space-between;align-items:center;
        border-bottom:1px solid var(--glass-border);user-select:none;-webkit-user-select:none;flex-shrink:0}
    .win-title{font-size:12px;font-weight:600;color:var(--text);
        display:flex;align-items:center;gap:7px;letter-spacing:.01em}
    .win-icon{font-size:14px;width:22px;height:22px;border-radius:5px;
        display:flex;align-items:center;justify-content:center}
    .win-controls{display:flex;gap:6px}
    .win-btn{width:12px;height:12px;border-radius:50%;cursor:pointer;flex-shrink:0;transition:opacity .15s}
    .win-btn.close{background:#ff5f57}.win-btn.min{background:#febc2e}.win-btn.max{background:#28c840}
    .win-content{padding:14px;overflow-y:auto;flex:1;min-height:0}
    .win-content::-webkit-scrollbar{width:5px}
    .win-content::-webkit-scrollbar-thumb{background:rgba(255,255,255,.14);border-radius:3px}
    /* ── Cards ── */
    .card{background:rgba(255,255,255,.035);border-radius:12px;padding:13px;
        margin-bottom:10px;border:1px solid var(--glass-border)}
    .card-title{font-size:10px;font-weight:700;color:var(--text-muted);text-transform:uppercase;
        letter-spacing:.8px;margin-bottom:9px;display:flex;align-items:center;gap:5px}
    .card-2col{display:grid;grid-template-columns:1fr 1fr;gap:9px}
    .card-grid{display:grid;grid-template-columns:repeat(auto-fill,minmax(100px,1fr));gap:7px}
    /* ── Sensor Tile ── */
    .sensor{background:rgba(0,0,0,.22);border-radius:10px;padding:11px 10px;
        position:relative;overflow:hidden;cursor:default}
    .sensor::before{content:'';position:absolute;top:0;left:0;right:0;height:2px;border-radius:10px 10px 0 0}
    .sensor.good::before{background:var(--accent)}.sensor.warn::before{background:var(--warning)}.sensor.danger::before{background:var(--danger)}
    .sensor-icon{font-size:16px;margin-bottom:3px;line-height:1}
    .sensor-label{font-size:9px;color:var(--text-muted);white-space:nowrap;overflow:hidden;text-overflow:ellipsis}
    .sensor-value{font-size:17px;font-weight:700;color:var(--text);line-height:1.2;margin:2px 0}
    .sensor-unit{font-size:9px;color:var(--text-dim)}
    /* ── Human-centric gauges ── */
    .gauge-wrap{background:rgba(0,0,0,.24);border:1px solid var(--glass-border);border-radius:12px;padding:12px 13px}
    .gauge-h{display:flex;justify-content:space-between;align-items:baseline;margin-bottom:8px}
    .gauge-label{font-size:10px;color:var(--text-muted);text-transform:uppercase;letter-spacing:.7px;font-weight:600}
    .gauge-val{font-size:14px;font-weight:800;font-family:'JetBrains Mono',monospace}
    /* outer track (grey shell) — marks go here, no overflow:hidden */
    .gauge-outer{position:relative;height:18px;display:flex;align-items:center}
    .gauge-track{height:10px;border-radius:999px;background:rgba(255,255,255,.07);width:100%;
        position:relative;overflow:hidden}
    /* coloured progress fill — ONLY to value */
    .gauge-fill{position:absolute;left:0;top:0;bottom:0;border-radius:999px;
        transition:width .6s cubic-bezier(.4,0,.2,1)}
    /* threshold tick marks — sit in .gauge-outer, above track */
    .gauge-mark{position:absolute;top:0;bottom:0;width:2px;border-radius:1px;
        background:rgba(255,255,255,.8);box-shadow:0 0 5px rgba(255,255,255,.7)}
    .gauge-foot{display:flex;justify-content:space-between;font-size:9px;color:var(--text-muted);margin-top:4px}
    .hero-grid{display:grid;grid-template-columns:1fr 1fr;gap:8px}
    .dash-mix{display:grid;grid-template-columns:1.1fr .9fr;gap:9px}
    /* ── Analogue clock — fixed ── */
    .analogue-clock{width:160px;height:160px;margin:0 auto;border-radius:50%;
        position:relative;overflow:hidden;
        background:radial-gradient(circle at 40% 30%, rgba(255,255,255,.13), rgba(0,0,0,.55));
        border:2px solid rgba(255,255,255,.1);
        box-shadow:inset 0 0 30px rgba(0,0,0,.5),0 8px 24px rgba(0,0,0,.5)}
    /* marks: placed at top of clock, rotate around clock center */
    .clk-mark{position:absolute;width:2px;height:8px;background:rgba(255,255,255,.45);
        left:calc(50% - 1px);top:calc(50% - 78px);
        transform-origin:1px 78px;border-radius:1px}
    .clk-mark.major{height:12px;top:calc(50% - 78px);width:3px;left:calc(50% - 1.5px);
        background:rgba(255,255,255,.75);transform-origin:1.5px 78px}
    .clk-hand{position:absolute;left:50%;top:50%;transform-origin:50% 100%;border-radius:4px 4px 2px 2px}
    .clk-hour{width:5px;height:42px;margin-left:-2.5px;margin-top:-42px;
        background:linear-gradient(to top,rgba(255,255,255,.9),rgba(255,255,255,.5))}
    .clk-min{width:3px;height:58px;margin-left:-1.5px;margin-top:-58px;
        background:linear-gradient(to top,#93c5fd,rgba(147,197,253,.5))}
    .clk-sec{width:2px;height:66px;margin-left:-1px;margin-top:-54px;
        background:#f97316;box-shadow:0 0 8px rgba(249,115,22,.8)}
    .clk-tail{position:absolute;width:2px;height:12px;left:calc(50% - 1px);top:50%;
        background:#f97316;transform-origin:1px 0;border-radius:0 0 2px 2px}
    .clk-dot{position:absolute;left:50%;top:50%;width:9px;height:9px;
        margin:-4.5px 0 0 -4.5px;border-radius:50%;
        background:#fff;box-shadow:0 0 6px rgba(255,255,255,.6)}
    .clk-rim{position:absolute;inset:3px;border-radius:50%;
        border:1px solid rgba(255,255,255,.07);pointer-events:none}
    .kv-table{width:100%;border-collapse:collapse;font-size:11px}
    .kv-table td{padding:6px 4px;border-bottom:1px solid rgba(255,255,255,.06)}
    .kv-k{color:var(--text-dim)}
    .kv-v{text-align:right;color:var(--text);font-weight:600;font-family:'JetBrains Mono',monospace}
    /* ── Battery donut ── */
    .bat-donut{display:flex;flex-direction:column;align-items:center;justify-content:center;gap:6px}
    .bat-donut svg{filter:drop-shadow(0 0 8px rgba(0,212,170,.3))}
    .bat-center{position:absolute;text-align:center;pointer-events:none}
    .bat-pct{font-size:24px;font-weight:900;color:#fff;line-height:1}
    .bat-sub{font-size:10px;color:rgba(255,255,255,.45);margin-top:1px}
    .bat-row{display:flex;gap:12px;justify-content:center;flex-wrap:wrap}
    .bat-kv{display:flex;flex-direction:column;align-items:center;gap:1px}
    .bat-kv span:first-child{font-size:9px;color:var(--text-muted);text-transform:uppercase;letter-spacing:.5px}
    .bat-kv span:last-child{font-size:13px;font-weight:700;color:var(--text);font-family:'JetBrains Mono',monospace}
    /* ── AQI Dial ── */
    .aqi-wrap{display:flex;flex-direction:column;align-items:center;gap:4px}
    .aqi-label{font-size:11px;font-weight:700;letter-spacing:.5px}
    /* ── Wind rose ── */
    .wind-wrap{display:flex;flex-direction:column;align-items:center;gap:6px}
    .wind-info{display:flex;gap:16px;justify-content:center}
    .wind-kv{display:flex;flex-direction:column;align-items:center}
    .wind-kv span:first-child{font-size:9px;color:var(--text-muted);text-transform:uppercase;letter-spacing:.5px}
    .wind-kv span:last-child{font-size:14px;font-weight:700;color:var(--text)}
    /* ── Pressure trend ── */
    .press-card{display:flex;align-items:center;gap:12px;padding:10px;
        background:rgba(0,0,0,.22);border-radius:10px;border:1px solid var(--glass-border)}
    .press-arrow{font-size:36px;line-height:1}
    .press-info{flex:1}
    .press-val{font-size:22px;font-weight:800;color:var(--text)}
    .press-forecast{font-size:11px;color:var(--text-dim);margin-top:2px}
    .press-delta{font-size:10px;margin-top:1px}
    /* ── Fault matrix ── */
    .fault-grid{display:grid;grid-template-columns:repeat(auto-fill,minmax(88px,1fr));gap:5px}
    .fault-item{display:flex;align-items:center;gap:5px;padding:5px 7px;
        background:rgba(0,0,0,.20);border-radius:7px;font-size:10px;color:var(--text-dim)}
    .fault-dot{width:7px;height:7px;border-radius:50%;flex-shrink:0}
    .fault-dot.ok{background:#22c55e;box-shadow:0 0 5px #22c55e88}
    .fault-dot.bad{background:#ef4444;box-shadow:0 0 5px #ef444488;animation:pulse-bad .8s infinite alternate}
    .fault-dot.na{background:#475569}
    @keyframes pulse-bad{from{opacity:1}to{opacity:.4}}
    /* ── Spark-in-tile ── */
    .sensor canvas.spark{position:absolute;bottom:0;left:0;right:0;height:28px;opacity:.45;border-radius:0 0 10px 10px}
    /* ── I2C scanner ── */
    .i2c-addr{display:inline-flex;align-items:center;gap:4px;background:rgba(255,255,255,.07);
        border-radius:6px;padding:2px 8px;margin:2px;white-space:nowrap;position:relative;cursor:default}
    .i2c-dot{width:7px;height:7px;border-radius:50%;flex-shrink:0}
    .i2c-dot.ok{background:#22c55e;box-shadow:0 0 5px #22c55e88}
    .i2c-dot.bad{background:#ef4444;box-shadow:0 0 5px #ef444488}
    .i2c-dot.unknown{background:#f59e0b;box-shadow:0 0 5px #f59e0b88}
    .i2c-dot.unassigned{background:#475569}
    .i2c-qmark{width:14px;height:14px;border-radius:50%;background:rgba(255,255,255,.12);
        color:rgba(255,255,255,.55);font-size:9px;font-weight:700;display:inline-flex;
        align-items:center;justify-content:center;cursor:help;flex-shrink:0}
    .i2c-tt{display:none;position:absolute;bottom:calc(100% + 6px);left:50%;transform:translateX(-50%);
        background:rgba(12,12,24,.97);border:1px solid rgba(255,255,255,.14);border-radius:8px;
        padding:7px 11px;z-index:9000;min-width:160px;max-width:260px;pointer-events:none;
        box-shadow:0 8px 24px rgba(0,0,0,.6);backdrop-filter:blur(12px)}
    .i2c-tt::after{content:'';position:absolute;top:100%;left:50%;transform:translateX(-50%);
        border:6px solid transparent;border-top-color:rgba(255,255,255,.14)}
    .i2c-qmark:hover .i2c-tt,.i2c-addr:hover .i2c-tt{display:block}
    .i2c-tt-name{font-size:12px;font-weight:700;color:#e2e8f0;margin-bottom:3px}
    .i2c-tt-addr{font-size:10px;color:#64748b;font-family:'JetBrains Mono',monospace}
    .i2c-tt-status{font-size:10px;margin-top:4px;font-weight:600}
    .i2c-tt-desc{font-size:10px;color:#94a3b8;margin-top:2px;line-height:1.4}
    /* ── Mode buttons active ── */
    .mode-btn{transition:all .2s}
    .mode-btn.mode-active{background:rgba(0,212,170,.22)!important;border-color:rgba(0,212,170,.6)!important;
        color:var(--accent)!important;box-shadow:0 0 12px rgba(0,212,170,.25)}
    /* ── Discovery report ── */
    .disc-item{padding:8px 10px;border-radius:8px;margin-bottom:6px;font-size:11px;
        background:rgba(0,0,0,.2);border:1px solid rgba(255,255,255,.06)}
    .disc-found{border-left:3px solid #22c55e}
    .disc-miss{border-left:3px solid #ef4444;opacity:.7}
    /* ── Sensor List rows ── */
    .srow{display:flex;justify-content:space-between;align-items:center;
        padding:6px 0;border-bottom:1px solid rgba(255,255,255,.05);font-size:11px;
        cursor:pointer;transition:background .1s;border-radius:4px}
    .srow:hover{background:rgba(255,255,255,.04)}
    .srow span:first-child{color:var(--text-dim);flex:1;padding-right:8px}
    .srow span:last-child{color:var(--text);font-weight:600;white-space:nowrap;font-family:'JetBrains Mono',monospace}
    /* ── Sparkline chart ── */
    .chartbox{background:rgba(0,0,0,.25);border-radius:8px;padding:8px 10px;margin-top:6px;
        position:relative;height:var(--chart-h);overflow:hidden}
    .chartbox canvas{position:absolute;inset:0;width:100%;height:100%}
    .chartbox .chart-label{font-size:9px;color:var(--text-muted);position:absolute;top:5px;left:8px;z-index:1}
    .chartbox .chart-cur{font-size:13px;font-weight:700;color:var(--accent);
        position:absolute;bottom:5px;right:8px;z-index:1;font-family:'JetBrains Mono',monospace}
    /* ── Terminal ── */
    #log-terminal{
        background:rgba(0,0,0,.6);color:#00ff88;
        font-family:'JetBrains Mono','Fira Code',monospace;
        font-size:11px;line-height:1.55;
        padding:12px;border-radius:10px;
        white-space:pre-wrap;word-break:break-all;
        overflow-y:auto;
        min-height:320px;
        max-height:min(70vh,600px);
        border:1px solid rgba(0,255,136,.12);
        cursor:text;
        /* allow text selection for copy */
        user-select:text;-webkit-user-select:text
    }
    #log-terminal ::selection{background:rgba(0,212,170,.35);color:#fff}
    .log-toolbar{display:flex;gap:6px;margin-bottom:8px;align-items:center}
    .log-search{flex:1;background:rgba(0,0,0,.35);color:var(--text);
        border:1px solid var(--glass-border);border-radius:7px;
        padding:5px 10px;font-family:'JetBrains Mono',monospace;font-size:11px;outline:none}
    .log-search:focus{border-color:var(--accent-dim)}
    /* ── Buttons ── */
    .btn{background:rgba(255,255,255,.06);color:var(--text);
        border:1px solid var(--glass-border);padding:9px 14px;border-radius:9px;
        margin:2px;font-family:inherit;font-size:12px;cursor:pointer;
        transition:all .18s;display:inline-flex;align-items:center;gap:5px}
    .btn:hover{background:rgba(255,255,255,.11);transform:translateY(-1px)}
    .btn:active{transform:translateY(0)}
    .btn.danger{background:rgba(255,71,87,.14);border-color:rgba(255,71,87,.28);color:#ff6b7a}
    .btn.danger:hover{background:rgba(255,71,87,.24)}
    .btn.accent{background:rgba(0,212,170,.14);border-color:rgba(0,212,170,.28);color:var(--accent)}
    .btn.accent:hover{background:rgba(0,212,170,.24)}
    .btn-group{display:flex;flex-wrap:wrap;gap:3px;margin:7px 0}
    /* ── Input ── */
    input,select{background:rgba(0,0,0,.3);color:var(--text);border:1px solid var(--glass-border);
        padding:9px 11px;border-radius:8px;font-family:inherit;font-size:12px;outline:none;width:100%}
    input:focus,select:focus{border-color:var(--accent-dim)}
    /* ── Status bar ── */
    .status-bar{position:fixed;top:10px;right:12px;display:flex;gap:7px;z-index:200}
    .status-pill{background:rgba(18,18,30,.8);backdrop-filter:blur(10px);
        padding:5px 11px;border-radius:18px;font-size:10px;
        display:flex;align-items:center;gap:5px;border:1px solid var(--glass-border)}
    .status-dot{width:6px;height:6px;border-radius:50%;background:var(--accent)}
    .status-dot.offline{background:var(--danger)}
    #ota-prog{text-align:center;margin-top:8px;font-size:11px;color:var(--accent)}
    /* ── Copy toast ── */
    .toast{position:fixed;bottom:70px;left:50%;transform:translateX(-50%) translateY(8px);
        background:rgba(0,212,170,.18);border:1px solid rgba(0,212,170,.35);color:var(--accent);
        font-size:11px;padding:6px 14px;border-radius:20px;opacity:0;
        transition:all .25s;z-index:9999;pointer-events:none}
    .toast.show{opacity:1;transform:translateX(-50%) translateY(0)}
    /* ── Mobile ── */
    @media(max-width:600px){
        #desktop{padding:8px;padding-bottom:64px}
        .window{min-width:calc(100vw - 16px) !important}
        .card-grid{grid-template-columns:repeat(2,1fr)}
        .card-2col{grid-template-columns:1fr}
    }
</style>
</head>
<body>
<div class="status-bar">
    <div class="status-pill"><div class="status-dot" id="wifiDot"></div><span id="wifiStatus">…</span></div>
    <div class="status-pill">🌡️ <span id="quickTemp">--</span>°C &nbsp;💧<span id="quickHum">--</span>%</div>
    <div class="status-pill" id="langPill" style="cursor:pointer;gap:5px;transition:background .2s" title="Toggle language PL/EN" onclick="setLang(LANG==='en'?'pl':'en')">
        <span id="langFlag" style="font-size:16px;line-height:1"></span>
        <span id="langLbl" style="font-size:10px;font-weight:700;letter-spacing:.3px"></span>
    </div>
    <div class="status-pill"><span id="clock">--:--</span></div>
</div>
<div id="desktop"></div>
<div class="dock" id="dock"></div>
<div class="toast" id="toast">Copied!</div>

<script>
// ============================================================
// ATLAS-OS v2.3 — glass window desktop + charts + wide logs
// ============================================================
const DOCK_BTNS = [
    {id:'dash',    icon:'◉',   title:'Dashboard'},
    {id:'medical', icon:'🏥',  title:'Medical'},
    {id:'space',   icon:'🌌',  title:'Space Weather'},
    {id:'environ', icon:'🌿',  title:'Environment'},
    {id:'spectr',  icon:'🌈',  title:'Spectrometer'},
    {id:'thermal', icon:'🌡️', title:'Thermal Camera'},
    {id:'imu',     icon:'🔭',  title:'IMU & Mag'},
    {id:'sensors', icon:'📡',  title:'All Sensors'},
    {id:'charts',  icon:'📈',  title:'Charts'},
    {id:'sys',     icon:'⚙️', title:'System'},
    {id:'log',     icon:'📜',  title:'Logs'},
];
const ICON_COLORS = {
    '◉':'#00d4aa','🏥':'#fb7185','🌌':'#818cf8','🌿':'#4ade80',
    '🌈':'#f472b6','🌡️':'#f97316','🔭':'#60a5fa','📡':'#a78bfa',
    '📈':'#f59e0b','⚙️':'#fbbf24','📜':'#94a3b8'
};

// ── i18n language ──
let LANG = localStorage.getItem('atlas_lang') || 'en';
const T = {
    en: {
        dashboard:'Dashboard', medical:'Medical', space:'Space Weather',
        environ:'Environment', spectr:'Spectrometer', thermal:'Thermal Camera',
        imu:'IMU & Mag', sensors:'All Sensors', charts:'Charts',
        sys:'System', log:'Logs',
        feels:'Feels Like', temp:'Temperature', hum:'Humidity',
        press:'Pressure', dew:'Dew Point', cloud:'Cloud Base',
        battery:'Battery', solar:'Solar', iaq:'Air Quality', co2:'CO₂',
        uvi:'UV Index', radiation:'Radiation', wind:'Wind',
        migraine:'Migraine Risk', rheum:'Rheumatic Risk', sinus:'Sinus Risk',
        utci:'Thermal Comfort', radon:'Radon Risk', lung:'Lung Deposition',
        aurora:'Aurora Probability', kindex:'K-Index', ozone:'Ozone',
        forbush:'Forbush Decrease', ssc:'CME / SSC', skyq:'Sky Condition',
        aod:'Aerosol Depth', ghi:'Solar Irradiance',
        biometeo:'Biometeo Alert', bpi:'Baro Pain Index',
    },
    pl: {
        dashboard:'Panel Główny', medical:'Medyczny', space:'Pogoda Kosmiczna',
        environ:'Środowisko', spectr:'Spektrometr', thermal:'Kamera Termiczna',
        imu:'IMU & Mag', sensors:'Wszystkie Czujniki', charts:'Wykresy',
        sys:'System', log:'Logi',
        feels:'Odczuwalna', temp:'Temperatura', hum:'Wilgotność',
        press:'Ciśnienie', dew:'Punkt Rosy', cloud:'Podstawa Chmur',
        battery:'Bateria', solar:'Solar', iaq:'Jakość Powietrza', co2:'CO₂',
        uvi:'Indeks UV', radiation:'Promieniowanie', wind:'Wiatr',
        migraine:'Ryzyko Migreny', rheum:'Ryzyko Reumatyczne', sinus:'Ryzyko Zatok',
        utci:'Komfort Cieplny', radon:'Ryzyko Radonu', lung:'Depozycja Płucna',
        aurora:'Prawdopodob. Zorzy', kindex:'Indeks K', ozone:'Ozon',
        forbush:'Spadek Forbusha', ssc:'CME / SSC', skyq:'Stan Nieba',
        aod:'Głębokość Aerozol.', ghi:'Nasl. Słoneczne',
        biometeo:'Alert Biometeo', bpi:'Barometryczny Ból',
    }
};
function t(key){ return (T[LANG]||T.en)[key] || key; }
function setLang(l){ LANG=l; localStorage.setItem('atlas_lang',l); location.reload(); }

let zIdx = 100;
const desktop = document.getElementById('desktop');
const dock    = document.getElementById('dock');

const $  = id => document.getElementById(id);
const el = (tag,cls,html) => { const e=document.createElement(tag); if(cls) e.className=cls; if(html) e.innerHTML=html; return e; };

// ── Clock ──
const updateClock = () => $('clock').textContent = new Date().toLocaleTimeString([],{hour:'2-digit',minute:'2-digit'});
setInterval(updateClock,1000); updateClock();
setInterval(tickDashClock,1000);

// ── WiFi + quick readings ──
function pollStatus(){
    fetch('/wifi').then(r=>r.text()).then(t=>{
        const ok = t.includes('OK');
        $('wifiDot').className = 'status-dot'+(ok?'':' offline');
        $('wifiStatus').textContent = ok?'Online':'Offline';
    }).catch(()=>{ $('wifiDot').className='status-dot offline'; $('wifiStatus').textContent='Offline'; });
    fetch('/api').then(r=>r.json()).then(d=>{
        const s = d.sensors||{};
        $('quickTemp').textContent = s.SHT45_Temp!=null ? (+s.SHT45_Temp).toFixed(1) : '--';
        $('quickHum').textContent  = s.SHT45_Hum !=null ? (+s.SHT45_Hum).toFixed(0)  : '--';
    }).catch(()=>{});
}
setInterval(pollStatus,10000); pollStatus();

// ── Toast copy notification ──
function showToast(msg='Copied!'){
    const t=$('toast'); t.textContent=msg; t.classList.add('show');
    setTimeout(()=>t.classList.remove('show'),1800);
}

// ── Dock ──
DOCK_BTNS.forEach((b,i)=>{
    const d = el('div','dock-item',b.icon);
    d.title=b.title; d.dataset.id=b.id;
    d.onclick=()=>loadSection(b.id);
    dock.appendChild(d);
});
function setActiveBtn(id){
    dock.querySelectorAll('.dock-item').forEach(d=>d.classList.toggle('active',d.dataset.id===id));
}

// ── Window factory ──
function createWindow(id, title, icon, x, y, content, w=340) {
    if($(id)){ $(id).style.display='flex'; focusWin(id); return $(id); }
    const col = ICON_COLORS[icon]||'#888';
    const win = el('div','window');
    win.id=id;
    const vw=window.innerWidth, vh=window.innerHeight;
    win.style.cssText = `left:${Math.min(x,vw-w-10)}px;top:${Math.min(y,vh-300)}px;width:${Math.min(w,vw-16)}px;z-index:${++zIdx};max-height:${vh-80}px`;
    win.innerHTML = `
        <div class="win-header" onmousedown="dragStart(event,'${id}')" ontouchstart="dragStart(event,'${id}')">
            <div class="win-title">
                <span class="win-icon" style="background:${col}22;color:${col}">${icon}</span>${title}
            </div>
            <div class="win-controls">
                <div class="win-btn min" onclick="minWin('${id}')"></div>
                <div class="win-btn max" onclick="maxWin('${id}')"></div>
                <div class="win-btn close" onclick="closeWin('${id}')"></div>
            </div>
        </div>
        <div class="win-content">${content}</div>`;
    desktop.appendChild(win);
    win.addEventListener('mousedown',()=>focusWin(id));
    return win;
}
function focusWin(id){ const w=$(id); if(w) w.style.zIndex=++zIdx; }
function minWin(id){ const w=$(id); if(w) w.style.display='none'; }
function maxWin(id){
    const w=$(id); if(!w) return;
    if(w._maximized){ Object.assign(w.style,w._prev||{}); w._maximized=false; }
    else {
        w._prev={left:w.style.left,top:w.style.top,width:w.style.width,maxHeight:w.style.maxHeight};
        Object.assign(w.style,{left:'8px',top:'8px',width:(window.innerWidth-16)+'px',maxHeight:(window.innerHeight-72)+'px'});
        w._maximized=true;
    }
}
function closeWin(id){ const w=$(id); if(w) w.remove(); }

// ── Drag ──
let _dw=null,_do={x:0,y:0};
function dragStart(e,id){
    e.preventDefault(); _dw=$(id); focusWin(id);
    const p=e.touches?e.touches[0]:e, r=_dw.getBoundingClientRect();
    _do={x:p.clientX-r.left,y:p.clientY-r.top};
    document.addEventListener('mousemove',dragMove);
    document.addEventListener('mouseup',dragEnd);
    document.addEventListener('touchmove',dragMove,{passive:false});
    document.addEventListener('touchend',dragEnd);
}
function dragMove(e){ if(!_dw) return; e.preventDefault();
    const p=e.touches?e.touches[0]:e;
    _dw.style.left=(p.clientX-_do.x)+'px';
    _dw.style.top =(p.clientY-_do.y)+'px';
}
function dragEnd(){ _dw=null;
    ['mousemove','mouseup','touchmove','touchend'].forEach(ev=>document.removeEventListener(ev,ev==='mousemove'||ev==='touchmove'?dragMove:dragEnd));
}

// ── Notify ──
function notify(msg,ok=true){
    const n=el('div','card');
    n.style.cssText=`position:fixed;top:56px;right:14px;z-index:10001;padding:9px 16px;font-size:12px;
        background:rgba(${ok?'0,212,170':'255,71,87'},.18);border-color:rgba(${ok?'0,212,170':'255,71,87'},.35);
        color:${ok?'var(--accent)':'var(--danger)'}`;
    n.textContent=msg; document.body.appendChild(n);
    setTimeout(()=>n.remove(),2200);
}

// ── API cmd ──
function cmd(action,extras=''){
    fetch('/cmd?action='+action+extras).then(r=>r.text()).then(()=>notify('✓ '+action)).catch(()=>notify('✗ failed',false));
}

// ── Init language pill ──
(function initLangPill(){
    const f=$('langFlag'), l=$('langLbl'), p=$('langPill');
    if(LANG==='pl'){
        if(f) f.textContent='🇵🇱';
        if(l) l.textContent='PL';
        if(p) p.style.borderColor='rgba(220,36,31,.35)';
    } else {
        if(f) f.textContent='🇬🇧';
        if(l) l.textContent='EN';
        if(p) p.style.borderColor='rgba(0,82,180,.35)';
    }
})();

// ── Sensor tile builder ──
function sensorTile(icon,label,val,unit,status='good'){
    return `<div class="sensor ${status}">
        <div class="sensor-icon">${icon}</div>
        <div class="sensor-label">${label}</div>
        <div class="sensor-value">${val??'--'}</div>
        <div class="sensor-unit">${unit}</div>
    </div>`;
}
function fv(v,dp=1){ return (v!=null&&v!==undefined)?(+v).toFixed(dp):'--'; }
function fvi(v){ return (v!=null&&v!==undefined)?Math.round(+v):'--'; }
function status_iaq(v){ return v>200?'danger':v>100?'warn':'good'; }
function status_co2(v){ return v>2000?'danger':v>1000?'warn':'good'; }
function status_pm(v){  return v>55?'danger':v>35?'warn':'good'; }
function status_rad(v){ return v>0.5?'danger':v>0.2?'warn':'good'; }
function status_num(v,warn,danger){ return (v>=danger)?'danger':(v>=warn)?'warn':'good'; }

function clamp(v,min,max){ return Math.max(min,Math.min(max,v)); }
function gauge(label,val,min,max,warn,danger,unit=''){
    const n = (val!=null && val!==undefined) ? +val : NaN;
    const safe = Number.isFinite(n);
    const cur = safe ? clamp(n,min,max) : min;
    const pct = ((cur-min)/(max-min))*100;
    const warnPct = ((warn-min)/(max-min))*100;
    const dangerPct = ((danger-min)/(max-min))*100;
    // fill colour = position-based: green → amber → red
    const col = !safe ? '#475569' : (n>=danger?'#ef4444':n>=warn?'#f59e0b':'#22c55e');
    // gradient on fill: always green→col so it looks smooth
    const fillGrad = !safe ? '#475569' : (n>=danger
        ? 'linear-gradient(90deg,#22c55e,#f59e0b 45%,#ef4444)'
        : n>=warn
        ? 'linear-gradient(90deg,#22c55e,#f59e0b)'
        : 'linear-gradient(90deg,#22c55e,#4ade80)');
    const dispVal = safe ? (n<10&&n%1!==0?n.toFixed(1):n<100?n.toFixed(1):Math.round(n)) : '--';
    return `<div class="gauge-wrap">
        <div class="gauge-h">
            <div class="gauge-label">${label}</div>
            <div class="gauge-val" style="color:${col}">${dispVal}${unit?' <span style="font-size:10px;font-weight:400;color:var(--text-dim)">'+unit+'</span>':''}</div>
        </div>
        <div class="gauge-outer">
            <div class="gauge-track" style="flex:1">
                <div class="gauge-fill" style="width:${pct}%;background:${fillGrad}"></div>
            </div>
            <div class="gauge-mark" style="left:${warnPct}%;top:2px;bottom:2px" title="Warn: ${warn}${unit}"></div>
            <div class="gauge-mark" style="left:${dangerPct}%;top:0;bottom:0;background:#ef4444;box-shadow:0 0 6px #ef444488" title="Danger: ${danger}${unit}"></div>
        </div>
        <div class="gauge-foot"><span>${min}${unit}</span><span>${max}${unit}</span></div>
    </div>`;
}

function analogClockHTML(){
    let marks='';
    for(let i=0;i<60;i++){
        const isMaj=(i%5===0);
        marks+=`<div class="clk-mark${isMaj?' major':''}" style="transform:rotate(${i*6}deg)"></div>`;
    }
    return `<div class="analogue-clock" id="dashClock">
        <div class="clk-rim"></div>
        ${marks}
        <div class="clk-hand clk-hour" id="clkHour"></div>
        <div class="clk-hand clk-min"  id="clkMin"></div>
        <div class="clk-hand clk-sec"  id="clkSec"></div>
        <div class="clk-tail"          id="clkTail"></div>
        <div class="clk-dot"></div>
    </div>`;
}
function tickDashClock(){
    const now = new Date();
    const sec = now.getSeconds(), ms = now.getMilliseconds();
    const min = now.getMinutes();
    const hr  = now.getHours()%12;
    const hEl=$('clkHour'), mEl=$('clkMin'), sEl=$('clkSec'), tEl=$('clkTail');
    if(!hEl||!mEl||!sEl) return;
    // smooth second hand
    const sDeg = sec*6 + ms*0.006;
    hEl.style.transform=`rotate(${hr*30 + min*0.5 + sec*0.00833}deg)`;
    mEl.style.transform=`rotate(${min*6 + sec*0.1}deg)`;
    sEl.style.transform=`rotate(${sDeg}deg)`;
    if(tEl) tEl.style.transform=`rotate(${sDeg}deg)`;
}

// ══════════════════════════════════════════════════════
// BATTERY RING DONUT  — SVG arc, no libs
// ══════════════════════════════════════════════════════
function batteryDonut(soc, volt, solar_mw, esp_mw, drain_rate, tte_min){
    const R=54, CX=64, CY=64, stroke=10;
    const circ=2*Math.PI*R;
    const safe=Number.isFinite(+soc)&&+soc>=0;
    const pct=safe?Math.min(100,Math.max(0,+soc)):0;
    const off=circ*(1-pct/100);
    const col=pct>60?'#22c55e':pct>25?'#f59e0b':'#ef4444';
    const glow=pct>60?'#22c55e55':pct>25?'#f59e0b55':'#ef444455';
    const drStr=Number.isFinite(+drain_rate)?(+drain_rate>0?'+'+fv(drain_rate,1):fv(drain_rate,1))+'%/h':'--';
    const tteStr=Number.isFinite(+tte_min)&&+tte_min>0?(+tte_min/60).toFixed(1)+'h':'--';
    return `<div class="bat-donut">
        <div style="position:relative;width:128px;height:128px">
            <svg width="128" height="128" viewBox="0 0 128 128">
                <defs><filter id="gf"><feGaussianBlur stdDeviation="2"/></filter></defs>
                <circle cx="${CX}" cy="${CY}" r="${R}" fill="none" stroke="rgba(255,255,255,.07)" stroke-width="${stroke}"/>
                <circle cx="${CX}" cy="${CY}" r="${R}" fill="none" stroke="${glow}" stroke-width="${stroke+6}" filter="url(#gf)" stroke-dasharray="${circ*pct/100} ${circ}" stroke-dashoffset="${circ/4}" stroke-linecap="round"/>
                <circle cx="${CX}" cy="${CY}" r="${R}" fill="none" stroke="${col}" stroke-width="${stroke}" stroke-dasharray="${circ*pct/100} ${circ}" stroke-dashoffset="${circ/4}" stroke-linecap="round" style="transition:stroke-dasharray .8s ease"/>
            </svg>
            <div class="bat-center" style="top:0;left:0;right:0;bottom:0;display:flex;flex-direction:column;align-items:center;justify-content:center">
                <div class="bat-pct" style="color:${col}">${safe?Math.round(pct):'--%'}<span style="font-size:13px">%</span></div>
                <div class="bat-sub">${Number.isFinite(+volt)?fv(volt,2)+' V':'-- V'}</div>
            </div>
        </div>
        <div class="bat-row">
            <div class="bat-kv"><span>Solar</span><span style="color:#fbbf24">${Number.isFinite(+solar_mw)?fv(solar_mw,0)+' mW':'--'}</span></div>
            <div class="bat-kv"><span>Load</span><span style="color:#f87171">${Number.isFinite(+esp_mw)?fv(esp_mw,0)+' mW':'--'}</span></div>
            <div class="bat-kv"><span>Rate</span><span style="color:${(+drain_rate||0)>0?'#4ade80':'#fb923c'}">${drStr}</span></div>
            <div class="bat-kv"><span>ETA</span><span>${tteStr}</span></div>
        </div>
    </div>`;
}

// ══════════════════════════════════════════════════════
// AQI SEMI-CIRCLE SPEEDOMETER  — canvas, WHO bands
// ══════════════════════════════════════════════════════
function drawAqiDial(canvasId, iaq){
    const cvs=$(canvasId); if(!cvs) return;
    const W=200, H=120;
    cvs.width=W; cvs.height=H;
    const ctx=cvs.getContext('2d');
    const cx=W/2, cy=H-10, r=88;
    const bands=[
        {lo:0,  hi:50,  col:'#22c55e', lbl:'Excellent'},
        {lo:50, hi:100, col:'#84cc16', lbl:'Good'},
        {lo:100,hi:150, col:'#f59e0b', lbl:'Moderate'},
        {lo:150,hi:200, col:'#f97316', lbl:'Poor'},
        {lo:200,hi:250, col:'#ef4444', lbl:'Bad'},
        {lo:250,hi:350, col:'#9333ea', lbl:'Hazardous'},
    ];
    const MAX=350;
    const toAng=(v)=>Math.PI+(Math.PI*Math.min(v,MAX)/MAX);
    // draw bands
    bands.forEach(b=>{
        ctx.beginPath();
        ctx.arc(cx,cy,r,toAng(b.lo),toAng(b.hi));
        ctx.lineWidth=16; ctx.strokeStyle=b.col+'cc'; ctx.stroke();
    });
    // track bg
    ctx.beginPath(); ctx.arc(cx,cy,r-22,Math.PI,2*Math.PI);
    ctx.lineWidth=1; ctx.strokeStyle='rgba(255,255,255,.08)'; ctx.stroke();
    // needle
    const val=Number.isFinite(+iaq)?Math.min(+iaq,MAX):0;
    const ang=toAng(val);
    const nx=cx+Math.cos(ang)*(r-10), ny=cy+Math.sin(ang)*(r-10);
    ctx.beginPath(); ctx.moveTo(cx,cy); ctx.lineTo(nx,ny);
    ctx.lineWidth=3; ctx.strokeStyle='#fff'; ctx.lineCap='round'; ctx.stroke();
    ctx.beginPath(); ctx.arc(cx,cy,6,0,2*Math.PI);
    ctx.fillStyle='#fff'; ctx.fill();
    // value text
    const safe=Number.isFinite(+iaq);
    ctx.fillStyle='#fff'; ctx.font='bold 22px Inter,sans-serif'; ctx.textAlign='center';
    ctx.fillText(safe?Math.round(+iaq):'--', cx, cy-14);
    // label
    let lbl='No data';
    if(safe) { const b=bands.find(b=>val>=b.lo&&val<b.hi)||bands[bands.length-1]; lbl=b.lbl; }
    const lcol=safe?(bands.find(b=>val>=b.lo&&val<b.hi)||bands[bands.length-1]).col:'#888';
    ctx.fillStyle=lcol; ctx.font='bold 11px Inter,sans-serif';
    ctx.fillText(lbl, cx, cy+6);
}

// ══════════════════════════════════════════════════════
// WIND ROSE COMPASS  — SVG needle
// ══════════════════════════════════════════════════════
function windRose(dir_deg, speed_kph, gust_kph){
    const safe=Number.isFinite(+dir_deg);
    const ang=safe?+dir_deg:0;
    const dirs=['N','NE','E','SE','S','SW','W','NW'];
    const card=safe?dirs[Math.round(ang/45)%8]:'--';
    const spd=Number.isFinite(+speed_kph)?fv(speed_kph,1):'--';
    const gst=Number.isFinite(+gust_kph)?fv(gust_kph,1):'--';
    // SVG compass
    const marks=dirs.map((d,i)=>{
        const a=(i*45-90)*Math.PI/180;
        const r1=38, r2=43, tx=50+Math.cos(a)*48, ty=50+Math.sin(a)*48;
        return `<line x1="${50+Math.cos(a)*r1}" y1="${50+Math.sin(a)*r1}" x2="${50+Math.cos(a)*r2}" y2="${50+Math.sin(a)*r2}" stroke="rgba(255,255,255,.3)" stroke-width="1.5"/>
        <text x="${tx}" y="${ty}" text-anchor="middle" dominant-baseline="middle" fill="${d==='N'?'#f97316':'rgba(255,255,255,.5)'}" font-size="${d==='N'||d==='S'||d==='E'||d==='W'?8:6}" font-family="Inter,sans-serif" font-weight="${d==='N'?'bold':'normal'}">${d}</text>`;
    }).join('');
    const na=(ang-90)*Math.PI/180;
    const nx=50+Math.sin((ang)*Math.PI/180)*30, ny=50-Math.cos((ang)*Math.PI/180)*30;
    const needle=safe?`<line x1="50" y1="50" x2="${nx}" y2="${ny}" stroke="#ef4444" stroke-width="3" stroke-linecap="round"/>
    <line x1="50" y1="50" x2="${50-Math.sin(ang*Math.PI/180)*12}" y2="${50+Math.cos(ang*Math.PI/180)*12}" stroke="rgba(255,255,255,.4)" stroke-width="2" stroke-linecap="round"/>`:'';
    return `<div class="wind-wrap">
        <svg width="100" height="100" viewBox="0 0 100 100">
            <circle cx="50" cy="50" r="46" fill="rgba(0,0,0,.3)" stroke="rgba(255,255,255,.1)" stroke-width="1"/>
            <circle cx="50" cy="50" r="32" fill="none" stroke="rgba(255,255,255,.05)" stroke-width="1"/>
            ${marks}${needle}
            <circle cx="50" cy="50" r="4" fill="#fff"/>
        </svg>
        <div class="wind-info">
            <div class="wind-kv"><span>Speed</span><span>${spd} kph</span></div>
            <div class="wind-kv"><span>Gust</span><span style="color:#f59e0b">${gst} kph</span></div>
            <div class="wind-kv"><span>Dir</span><span style="color:#60a5fa">${card} ${safe?Math.round(ang)+'°':''}</span></div>
        </div>
    </div>`;
}

// ══════════════════════════════════════════════════════
// PRESSURE TREND + ZAMBRETTI FORECAST
// ══════════════════════════════════════════════════════
function pressureTrend(press_hpa, delta3h, trend_str){
    const p=Number.isFinite(+press_hpa)?+press_hpa:null;
    const d=Number.isFinite(+delta3h)?+delta3h:null;
    const arrows={Rising:'↑',Falling:'↓','Rapidly Rising':'⇈','Rapidly Falling':'⇊',Stable:'→'};
    let dir='→', col='#94a3b8', zam='Changeable';
    if(d!=null){
        if(d>3){dir='⇈';col='#22c55e';zam='Fair, improving';}
        else if(d>1){dir='↑';col='#4ade80';zam='Clearing, good';}
        else if(d>-1){dir='→';col='#94a3b8';zam='Settled, no change';}
        else if(d>-3){dir='↓';col='#f59e0b';zam='Unsettled, rain possible';}
        else{dir='⇊';col='#ef4444';zam='Deteriorating, rain/storm';}
    } else if(trend_str && arrows[trend_str]){ dir=arrows[trend_str]; }
    const dStr=d!=null?(d>0?'+':'')+fv(d,1)+' hPa/3h':'--';
    return `<div class="press-card">
        <div class="press-arrow" style="color:${col}">${dir}</div>
        <div class="press-info">
            <div class="press-val" style="color:${col}">${p!=null?fv(p,1):'--.--'} <span style="font-size:13px;font-weight:400;color:var(--text-dim)">hPa</span></div>
            <div class="press-forecast">${zam}</div>
            <div class="press-delta" style="color:${col}">${dStr}</div>
        </div>
    </div>`;
}

// ══════════════════════════════════════════════════════
// FAULT / HEALTH LED MATRIX
// ══════════════════════════════════════════════════════
function faultMatrix(s){
    const sensors=[
        ['SHT45','Fault_SHT45'],['BMP585','Fault_BMP585'],['BME690','Fault_BME690'],
        ['SCD41','Fault_SCD41'],['SGP41','Fault_SGP41'],['BMV080','Fault_BMV080'],
        ['TSL2591','Fault_TSL2591'],['LTR390','Fault_LTR390'],['VEML','Fault_VEML7700'],
        ['OPT4048','Fault_OPT4048'],['TCS','Fault_TCS34725'],['AS7343','Fault_AS7343'],
        ['AS7331','Fault_AS7331'],['Geiger','Fault_Geiger'],['AS3935','Fault_AS3935'],
        ['MLX640','Fault_MLX90640'],['INA219','Fault_INA219'],['MAX17048','Fault_MAX17048'],
        ['LSM6','Fault_LSM6DSOX'],['LIS3MDL','Fault_LIS3MDL'],['BMM350','Fault_BMM350'],
        ['ZMOD4510','Fault_ZMOD4510'],['ILPS','Fault_ILPS22QS'],['MS8607','Fault_MS8607'],
        ['RG15','Fault_RG15'],['I2C Mem','Fault_I2CMemory'],
    ];
    return sensors.map(([name,key])=>{
        const v=s[key];
        let cls='na', tip='N/A';
        if(v!=null){ cls=(String(v)==='true'||String(v)==='1'||String(v)==='ON')?'bad':'ok'; tip=cls==='bad'?'FAULT':'OK'; }
        return `<div class="fault-item" title="${name}: ${tip}"><div class="fault-dot ${cls}"></div><span>${name}</span></div>`;
    }).join('');
}

// ══════════════════════════════════════════════════════
// MINI SPARKLINE INSIDE SENSOR TILE
// ══════════════════════════════════════════════════════
function sensorTileSpark(icon,label,val,unit,status,histKey,color){
    const hist=CHART_HISTORY[histKey]||[];
    const cid='spk_'+histKey+'_'+Math.random().toString(36).slice(2,6);
    // draw after render
    requestAnimationFrame(()=>{
        const cvs=$(cid); if(!cvs||hist.length<2) return;
        const W=cvs.offsetWidth||90, H=28;
        cvs.width=W; cvs.height=H;
        const ctx=cvs.getContext('2d');
        const mn=Math.min(...hist), mx=Math.max(...hist), rng=mx-mn||1;
        const pts=hist.map((v,i)=>[W*i/(hist.length-1), H-H*((v-mn)/rng)*0.9]);
        const grad=ctx.createLinearGradient(0,0,0,H);
        grad.addColorStop(0,color+'66'); grad.addColorStop(1,color+'00');
        ctx.beginPath(); ctx.moveTo(pts[0][0],H);
        pts.forEach(p=>ctx.lineTo(p[0],p[1]));
        ctx.lineTo(pts[pts.length-1][0],H); ctx.closePath();
        ctx.fillStyle=grad; ctx.fill();
        ctx.beginPath(); pts.forEach((p,i)=>i?ctx.lineTo(p[0],p[1]):ctx.moveTo(p[0],p[1]));
        ctx.strokeStyle=color; ctx.lineWidth=1.5; ctx.stroke();
    });
    return `<div class="sensor ${status}" style="padding-bottom:30px">
        <div class="sensor-icon">${icon}</div>
        <div class="sensor-label">${label}</div>
        <div class="sensor-value">${val??'--'}</div>
        <div class="sensor-unit">${unit}</div>
        <canvas class="spark" id="${cid}"></canvas>
    </div>`;
}

// ══════════════════════════════════════════════════════
// ZMOD4510 OUTDOOR GAS PANEL
// ══════════════════════════════════════════════════════
function zmodPanel(s){
    const no2=s.ZMOD4510_NO2_ppb, o3=s.ZMOD4510_O3_ppb,
          eaqi=s.ZMOD4510_EPA_AQI, fast=s.ZMOD4510_FAST_AQI,
          smog=s.ZMOD4510_Smog_Index, o3r=s.ZMOD4510_O3_Risk, st=s.ZMOD4510_Status;
    const no2col=Number.isFinite(+no2)?(+no2>200?'#ef4444':+no2>100?'#f59e0b':'#22c55e'):'#888';
    const o3col=Number.isFinite(+o3)?(+o3>120?'#ef4444':+o3>70?'#f59e0b':'#22c55e'):'#888';
    const aqcol=Number.isFinite(+eaqi)?(+eaqi>150?'#ef4444':+eaqi>100?'#f59e0b':+eaqi>50?'#84cc16':'#22c55e'):'#888';
    return `<div style="display:grid;grid-template-columns:1fr 1fr;gap:8px">
        <div style="background:rgba(0,0,0,.22);border-radius:10px;padding:12px;border-left:3px solid ${no2col}">
            <div style="font-size:9px;color:var(--text-muted);text-transform:uppercase;letter-spacing:.5px;margin-bottom:4px">NO₂</div>
            <div style="font-size:26px;font-weight:800;color:${no2col};line-height:1">${Number.isFinite(+no2)?fv(no2,1):'--'}</div>
            <div style="font-size:9px;color:var(--text-dim);margin-top:2px">ppb &nbsp;·&nbsp; ${Number.isFinite(+s.ZMOD4510_NO2_ugm3)?fv(s.ZMOD4510_NO2_ugm3,1)+' µg/m³':'--'}</div>
        </div>
        <div style="background:rgba(0,0,0,.22);border-radius:10px;padding:12px;border-left:3px solid ${o3col}">
            <div style="font-size:9px;color:var(--text-muted);text-transform:uppercase;letter-spacing:.5px;margin-bottom:4px">O₃</div>
            <div style="font-size:26px;font-weight:800;color:${o3col};line-height:1">${Number.isFinite(+o3)?fv(o3,1):'--'}</div>
            <div style="font-size:9px;color:var(--text-dim);margin-top:2px">ppb &nbsp;·&nbsp; ${Number.isFinite(+s.ZMOD4510_O3_ugm3)?fv(s.ZMOD4510_O3_ugm3,1)+' µg/m³':'--'} &nbsp;·&nbsp; Risk: ${o3r??'--'}</div>
        </div>
    </div>
    <div style="display:grid;grid-template-columns:1fr 1fr 1fr;gap:6px;margin-top:6px">
        <div style="background:rgba(0,0,0,.18);border-radius:8px;padding:8px;text-align:center">
            <div style="font-size:9px;color:var(--text-muted);margin-bottom:2px">EPA AQI</div>
            <div style="font-size:20px;font-weight:800;color:${aqcol}">${Number.isFinite(+eaqi)?Math.round(+eaqi):'--'}</div>
        </div>
        <div style="background:rgba(0,0,0,.18);border-radius:8px;padding:8px;text-align:center">
            <div style="font-size:9px;color:var(--text-muted);margin-bottom:2px">Fast AQI</div>
            <div style="font-size:20px;font-weight:800;color:var(--text)">${Number.isFinite(+fast)?Math.round(+fast):'--'}</div>
        </div>
        <div style="background:rgba(0,0,0,.18);border-radius:8px;padding:8px;text-align:center">
            <div style="font-size:9px;color:var(--text-muted);margin-bottom:2px">Smog Idx</div>
            <div style="font-size:20px;font-weight:800;color:${Number.isFinite(+smog)&&+smog>50?'#f59e0b':'var(--text)'}">${Number.isFinite(+smog)?fv(smog,0):'--'}</div>
        </div>
    </div>
    <div style="margin-top:6px;font-size:10px;color:var(--text-dim);text-align:right">Sensor: ${st??'--'}</div>`;
}

// ── Big readable metric card — compact data card ──
function bigCard(icon,title,value,unit,note,color='#00d4aa',status=''){
    const col = status==='danger'?'#ef4444':status==='warn'?'#f59e0b':color;
    const bg  = status==='danger'?'rgba(239,68,68,.08)':status==='warn'?'rgba(245,158,11,.08)':'rgba(0,0,0,.18)';
    return `<div style="background:${bg};border-radius:11px;padding:13px 14px;
                        border:1px solid rgba(255,255,255,.07);border-left:3px solid ${col};
                        margin-bottom:7px;display:flex;align-items:center;gap:12px">
        <div style="font-size:22px;line-height:1;flex-shrink:0">${icon}</div>
        <div style="flex:1;min-width:0">
            <div style="font-size:9px;color:var(--text-muted);text-transform:uppercase;
                letter-spacing:.6px;font-weight:700;margin-bottom:1px">${title}</div>
            <div style="font-size:22px;font-weight:800;color:${col};line-height:1.1;
                font-family:'JetBrains Mono',monospace">${value}
                <span style="font-size:11px;font-weight:400;color:var(--text-dim)">${unit}</span>
            </div>
            ${note?`<div style="font-size:10px;color:var(--text-dim);margin-top:2px">${note}</div>`:''}
        </div>
        ${status==='danger'?'<div style="font-size:16px">⚠️</div>':status==='warn'?'<div style="font-size:16px">⚡</div>':''}
    </div>`;
}

// ── Risk progress bar — with threshold tick ──
function riskBar(label, val, maxVal=10, color='#00d4aa'){
    const n = typeof val==='number'?val:parseFloat(val)||0;
    const pct = Math.min(100,(n/maxVal)*100);
    const col = pct>70?'#ef4444':pct>40?'#f59e0b':color;
    const fillGrad = pct>70
        ? 'linear-gradient(90deg,#22c55e,#f59e0b 40%,#ef4444)'
        : pct>40 ? 'linear-gradient(90deg,#22c55e,#f59e0b)'
        : 'linear-gradient(90deg,#22c55e,#4ade80)';
    return `<div style="margin-bottom:11px">
        <div style="display:flex;justify-content:space-between;align-items:baseline;
            font-size:11px;margin-bottom:5px">
            <span style="color:var(--text-dim)">${label}</span>
            <span style="color:${col};font-weight:800;font-family:'JetBrains Mono',monospace">
                ${n.toFixed(1)}<span style="font-size:9px;opacity:.6"> /${maxVal}</span>
            </span>
        </div>
        <div style="position:relative;height:8px">
            <div style="width:100%;height:100%;border-radius:999px;
                background:rgba(255,255,255,.07)"></div>
            <div style="position:absolute;left:0;top:0;height:100%;border-radius:999px;
                background:${fillGrad};width:${pct}%;
                transition:width .7s cubic-bezier(.4,0,.2,1);
                box-shadow:0 0 8px ${col}66"></div>
        </div>
    </div>`;
}

// ── Load section ──
function loadSection(id){
    setActiveBtn(id);
    if(id==='dash')     loadDashboard();
    else if(id==='medical')  loadMedical();
    else if(id==='space')    loadSpaceWeather();
    else if(id==='environ')  loadEnviron();
    else if(id==='spectr')   loadSpectrometer();
    else if(id==='thermal')  loadThermal();
    else if(id==='imu')      loadIMU();
    else if(id==='sensors')  loadSensors();
    else if(id==='charts')   loadCharts();
    else if(id==='sys')      loadSys();
    else if(id==='log')      loadLogs();
}

// ═══════════════════════════════════════════════════════════════
// DASHBOARD  — full featured v3
// ═══════════════════════════════════════════════════════════════
let _dashTimer = null;
function loadDashboard(){
    const vw=window.innerWidth, W=Math.min(vw-16,480);
    createWindow('dash-win','Dashboard','◉',20,40,`
        <!-- ROW 1: Battery donut + AQI dial + Clock -->
        <div class="card-2col" style="margin-bottom:9px">
            <div class="card" style="padding:12px">
                <div class="card-title">🔋 Battery</div>
                <div id="dBatDonut"></div>
            </div>
            <div class="card" style="padding:12px">
                <div class="card-title">🍃 Air Quality Index</div>
                <div class="aqi-wrap">
                    <canvas id="dAqiDial" width="200" height="120" style="max-width:100%"></canvas>
                    <div id="dAqiLabel" class="aqi-label"></div>
                </div>
            </div>
        </div>
        <!-- ROW 2: Safety gauges -->
        <div class="card" style="margin-bottom:9px">
            <div class="card-title">🧭 Safety Gauges</div>
            <div class="hero-grid" id="dGauges"></div>
        </div>
        <!-- ROW 3: Pressure trend + Wind rose + Clock -->
        <div style="display:grid;grid-template-columns:1fr 1fr 1fr;gap:9px;margin-bottom:9px">
            <div class="card" style="padding:12px">
                <div class="card-title">🌡️ Pressure</div>
                <div id="dPressure"></div>
            </div>
            <div class="card" style="padding:12px">
                <div class="card-title">🌬️ Wind</div>
                <div id="dWind"></div>
            </div>
            <div class="card" style="padding:12px;text-align:center">
                <div class="card-title">🕒 Local Time</div>
                <div id="dClock"></div>
                <div id="dMode" style="margin-top:6px;font-size:10px;color:var(--text-dim)"></div>
            </div>
        </div>
        <!-- ROW 4: Environment tiles with sparklines -->
        <div class="card" style="margin-bottom:9px">
            <div class="card-title">🏠 Environment</div>
            <div class="card-grid" id="dEnv"></div>
        </div>
        <!-- ROW 5: Outdoor gas ZMOD4510 -->
        <div class="card" style="margin-bottom:9px">
            <div class="card-title">🏭 Outdoor Air — ZMOD4510 (NO₂ / O₃)</div>
            <div id="dZmod"></div>
        </div>
        <!-- ROW 6: Radiation & Light -->
        <div class="card" style="margin-bottom:9px">
            <div class="card-title">⚡ Radiation & Light</div>
            <div class="card-grid" id="dRad"></div>
        </div>
        <!-- ROW 7: Fault matrix -->
        <div class="card">
            <div class="card-title">🔧 Hardware Health</div>
            <div class="fault-grid" id="dFault"></div>
        </div>
    `,W);

    function renderDash(){
        fetch('/api').then(r=>r.json()).then(d=>{
            const s=d.sensors||{};

            // Battery donut
            const dbd=$('dBatDonut');
            if(dbd) dbd.innerHTML=batteryDonut(
                s.BMS_State_Of_Charge, s.BMS_Cell_Voltage,
                s.SOLAR_Power_mW, s.BMS_ESP_Power_mW,
                s.BMS_Drain_Rate||s.BMS_SOC_Rate, s.BMS_Time_To_Empty_min);

            // AQI dial
            drawAqiDial('dAqiDial', s.BME688_IAQ);
            const aqL=$('dAqiLabel');
            if(aqL){
                const v=+s.BME688_IAQ;
                const acc=s.BME688_IAQ_Accuracy!=null?(' acc:'+s.BME688_IAQ_Accuracy):'';
                aqL.textContent='IAQ '+fv(s.BME688_IAQ,0)+acc;
                aqL.style.color=v>200?'#ef4444':v>100?'#f59e0b':'#22c55e';
            }

            // Safety gauges
            const dg=$('dGauges');
            if(dg) dg.innerHTML=[
                gauge('IAQ',s.BME688_IAQ,0,300,100,200,''),
                gauge('CO₂',s.SCD41_CO2_ppm,400,2500,1000,2000,'ppm'),
                gauge('PM2.5',s.BMV080_PM2_5,0,150,35,55,'µg/m³'),
                gauge('UV',s.LTR390_UVI,0,11,3,7,''),
                gauge('Radiation',s.Geiger_uSvh,0,1.0,0.2,0.5,'µSv/h'),
                gauge('Humidity',s.SHT45_Hum,0,100,60,75,'%'),
            ].join('');

            // Pressure trend
            const dp=$('dPressure');
            if(dp) dp.innerHTML=pressureTrend(s.BMP585_Pressure_hPa, s.METEO_Pressure_3h_Delta, s.BMP585_Trend);

            // Wind
            const dw=$('dWind');
            if(dw) dw.innerHTML=windRose(s.WIND_Direction_Deg, s.WIND_Speed_Kph, s.WIND_Gust_Kph);

            // Clock
            const dc=$('dClock');
            if(dc && !dc._inited){ dc.innerHTML=analogClockHTML(); dc._inited=true;
                if(_dashTimer) clearInterval(_dashTimer);
                _dashTimer=setInterval(tickDashClock,1000); tickDashClock(); }
            const dm=$('dMode');
            if(dm) dm.innerHTML=`Mode: <b>${s.System_Mode||'--'}</b> &nbsp;·&nbsp; Up: ${s.System_Uptime_Hours!=null?fv(s.System_Uptime_Hours,1)+'h':'--'}`;

            // Environment tiles with sparklines
            const de=$('dEnv');
            if(de) de.innerHTML=[
                sensorTileSpark('🌡️','Temperature',fv(s.SHT45_Temp),'°C',s.SHT45_Temp>35?'danger':s.SHT45_Temp>28?'warn':'good','SHT45_Temp','#f97316'),
                sensorTileSpark('🌡️','Feels Like',fv(s.METEO_Feels_Like_C),'°C','good','METEO_Feels_Like_C','#fb923c'),
                sensorTileSpark('💧','Humidity',fv(s.SHT45_Hum),'%',s.SHT45_Hum>75?'warn':'good','SHT45_Hum','#38bdf8'),
                sensorTileSpark('📊','Pressure',fv(s.BMP585_Pressure_hPa,0),'hPa','good','BMP585_Pressure_hPa','#94a3b8'),
                sensorTileSpark('🫧','CO₂',fv(s.SCD41_CO2_ppm,0),'ppm',status_co2(s.SCD41_CO2_ppm),'SCD41_CO2_ppm','#a3e635'),
                sensorTileSpark('💨','VOC Idx',fv(s.SGP41_VOC_Index,0),'',s.SGP41_VOC_Index>150?'warn':'good','SGP41_VOC_Index','#c084fc'),
            ].join('');

            // ZMOD4510
            const dz=$('dZmod');
            if(dz) dz.innerHTML=zmodPanel(s);

            // Radiation & Light
            const dr=$('dRad');
            if(dr) dr.innerHTML=[
                sensorTileSpark('☢️','Radiation',fv(s.Geiger_uSvh,3),'µSv/h',status_rad(s.Geiger_uSvh),'Geiger_uSvh','#facc15'),
                sensorTile('☢️','CPM',fv(s.Geiger_CPM,0),'CPM','good'),
                sensorTileSpark('🌞','UV Index',fv(s.LTR390_UVI,1),'',s.LTR390_UVI>7?'danger':s.LTR390_UVI>3?'warn':'good','LTR390_UVI','#e879f9'),
                sensorTile('🔆','Lux',fv(s.TSL2591_Lux,0),'lx','good'),
                sensorTile('🌈','CCT',fv(s.OPTICS_CCT,0),'K','good'),
                sensorTile('🧲','Mag',fv(s.LIS3MDL_Mag_uT),'µT','good'),
            ].join('');

            // Fault matrix
            const df=$('dFault');
            if(df) df.innerHTML=faultMatrix(s);

        }).catch(()=>{});
    }

    renderDash();
    // auto-refresh data every 30 s (clock ticks every 1s via _dashTimer)
    if(window._dashDataTimer) clearInterval(window._dashDataTimer);
    window._dashDataTimer=setInterval(renderDash, 30000);
}

// ═══════════════════════════════════════════════
// ALL SENSORS — 2-column list with copy-on-click
// ═══════════════════════════════════════════════
function loadSensors(){
    const vw=window.innerWidth;
    const w=Math.min(vw-16, 720);
    createWindow('sensors-win','All Sensors','📡',20,40,`
        <input class="log-search" placeholder="🔍 Filter sensors..." id="sFilter" oninput="filterSensors()">
        <div id="sList" style="margin-top:8px"></div>
    `,w);
    fetch('/api').then(r=>r.json()).then(d=>{
        const s=d.sensors||{};
        window._sensorData=s;
        renderSensorList(s,'');
    });
}
function renderSensorList(s,q){
    const keys=Object.keys(s).sort().filter(k=>!q||k.toLowerCase().includes(q.toLowerCase())||String(s[k]).toLowerCase().includes(q.toLowerCase()));
    const half=Math.ceil(keys.length/2);
    const col=(arr)=>arr.map(k=>{
        let v=s[k];
        if(v==null) v='null';
        else if(typeof v==='number') v=v.toFixed(v>1000?0:v>100?1:2);
        return `<div class="srow" onclick="copySensor('${k}','${v}')">
            <span>${k}</span><span>${v}</span>
        </div>`;
    }).join('');
    $('sList').innerHTML=`<div style="display:grid;grid-template-columns:1fr 1fr;gap:0 16px">
        <div>${col(keys.slice(0,half))}</div>
        <div>${col(keys.slice(half))}</div>
    </div><div style="font-size:10px;color:var(--text-muted);margin-top:6px;text-align:right">${keys.length} sensors — click to copy</div>`;
}
function filterSensors(){ const q=($('sFilter')||{}).value||''; if(window._sensorData) renderSensorList(window._sensorData,q); }
function copySensor(k,v){
    navigator.clipboard?.writeText(k+': '+v).then(()=>showToast('Copied: '+k));
}

// ═══════════════════════════════════════════════
// ENVIRONMENT
// ═══════════════════════════════════════════════
function loadEnv(){
    createWindow('env-win','Environment','🌬️',60,60,`
        <div class="card">
            <div class="card-title">🌡️ Thermal Comfort</div>
            <div class="card-grid" id="eThermal"></div>
        </div>
        <div class="card-2col">
            <div class="card"><div class="card-title">💧 Humidity</div><div id="eHum"></div></div>
            <div class="card"><div class="card-title">🌬️ Pressure</div><div id="ePress"></div></div>
        </div>
        <div class="card"><div class="card-title">☀️ Radiation</div><div class="card-grid" id="eRad"></div></div>
        <div class="card"><div class="card-title">⚡ Power</div><div class="card-grid" id="ePwr"></div></div>
    `,390);
    fetch('/api').then(r=>r.json()).then(d=>{
        const s=d.sensors||{};
        $('eThermal').innerHTML=[
            sensorTile('🌡️','Actual',fv(s.SHT45_Temp),'°C','good'),
            sensorTile('🌡️','Feels Like',fv(s.METEO_Feels_Like_C),'°C','good'),
            sensorTile('🌡️','Heat Index',fv(s.METEO_Heat_Index),'°C','good'),
            sensorTile('💧','Wet Bulb',fv(s.METEO_Wet_Bulb_C),'°C','good'),
            sensorTile('⛅','Dew Point',fv(s.METEO_Dew_Point_C),'°C','good'),
            sensorTile('🌬️','Wind Chill',fv(s.WIND_Speed_Kph)?fv(s.METEO_Feels_Like_C):'--','°C','good'),
        ].join('');
        $('eHum').innerHTML=[
            sensorTile('💧','Rel. Hum.',fv(s.SHT45_Hum),'%',s.SHT45_Hum>75?'warn':'good'),
            sensorTile('💧','Abs. Hum.',fv(s.METEO_Abs_Hum_g_m3),'g/m³','good'),
            sensorTile('🦠','Mold Risk',s.METEO_Mold_Risk?'YES':'No','',s.METEO_Mold_Risk?'warn':'good'),
            sensorTile('🦠','Virus Risk',s.METEO_Virus_Risk?'HIGH':'Low','',s.METEO_Virus_Risk?'warn':'good'),
        ].join('');
        $('ePress').innerHTML=[
            sensorTile('📊','Station',fv(s.BMP585_Pressure_hPa,1),'hPa','good'),
            sensorTile('📊','Sea Level',fv(s.METEO_Sea_Level_Press_hPa,1),'hPa','good'),
            sensorTile('☁️','Cloud Base',fv(s.METEO_Cloud_Base_m,0),'m','good'),
            sensorTile('💨','Air Density',fv(s.METEO_Air_Density,4),'kg/m³','good'),
        ].join('');
        $('eRad').innerHTML=[
            sensorTile('☢️','uSv/h',fv(s.Geiger_uSvh,3),'µSv/h',status_rad(s.Geiger_uSvh)),
            sensorTile('☢️','CPM',fv(s.Geiger_CPM,0),'CPM','good'),
            sensorTile('🌞','UV Index',fv(s.LTR390_UVI),'',s.LTR390_UVI>7?'danger':s.LTR390_UVI>3?'warn':'good'),
            sensorTile('🔆','Lux',fv(s.TSL2591_Lux,0),'lx','good'),
        ].join('');
        $('ePwr').innerHTML=[
            sensorTile('🔋','Battery',fv(s.BMS_State_Of_Charge,0),'%',s.BMS_State_Of_Charge<20?'warn':'good'),
            sensorTile('☀️','Solar',fv(s.SOLAR_Power_mW,0),'mW','good'),
            sensorTile('⚡','Node',fv(s.BMS_ESP_Power_mW,0),'mW','good'),
            sensorTile('⏱️','T to Empty',fv(s.BMS_Time_To_Empty_min,0),'min','good'),
        ].join('');
    });
}

// ═══════════════════════════════════════════════
// IMU + MAGNETOMETERS
// ═══════════════════════════════════════════════
function loadIMU(){
    createWindow('imu-win','IMU & Magnetometers','🔭',80,60,`
        <div class="card-2col">
            <div class="card">
                <div class="card-title">🔩 LSM6DSOX Accelerometer</div>
                <div id="iAccel"></div>
            </div>
            <div class="card">
                <div class="card-title">🔄 LSM6DSOX Gyroscope</div>
                <div id="iGyro"></div>
            </div>
        </div>
        <div class="card-2col">
            <div class="card">
                <div class="card-title">🧲 LIS3MDL Magnetometer</div>
                <div id="iLis"></div>
            </div>
            <div class="card">
                <div class="card-title">🧲 BMM350 Precision Mag</div>
                <div id="iBmm"></div>
            </div>
        </div>
        <div class="card">
            <div class="card-title">📐 Derived Orientation</div>
            <div class="card-grid" id="iDerived"></div>
        </div>
    `,440);
    fetch('/api').then(r=>r.json()).then(d=>{
        const s=d.sensors||{};
        $('iAccel').innerHTML=[
            sensorTile('➡️','X',fv(s.LSM6_Accel_X,3),'m/s²','good'),
            sensorTile('⬆️','Y',fv(s.LSM6_Accel_Y,3),'m/s²','good'),
            sensorTile('⬇️','Z',fv(s.LSM6_Accel_Z,3),'m/s²','good'),
            sensorTile('📏','|A|',fv(s.LSM6_Accel_Mag,3),'m/s²',s.LSM6_Shock_Det?'danger':'good'),
        ].join('');
        $('iGyro').innerHTML=[
            sensorTile('🔄','Gx',fv(s.LSM6_Gyro_X_rads,4),'rad/s','good'),
            sensorTile('🔄','Gy',fv(s.LSM6_Gyro_Y_rads,4),'rad/s','good'),
            sensorTile('🔄','Gz',fv(s.LSM6_Gyro_Z_rads,4),'rad/s','good'),
            sensorTile('🌡️','Die T',fv(s.LSM6_Temp_C),'°C','good'),
        ].join('');
        $('iLis').innerHTML=[
            sensorTile('🧲','X',fv(s.LIS3MDL_X_uT),'µT','good'),
            sensorTile('🧲','Y',fv(s.LIS3MDL_Y_uT),'µT','good'),
            sensorTile('🧲','Z',fv(s.LIS3MDL_Z_uT),'µT','good'),
            sensorTile('🧭','Hdg',fv(s.LIS3MDL_Heading,1),'°','good'),
        ].join('');
        $('iBmm').innerHTML=[
            sensorTile('🧲','X',fv(s.BMM350_Mag_X),'µT','good'),
            sensorTile('🧲','Y',fv(s.BMM350_Mag_Y),'µT','good'),
            sensorTile('🧲','Z',fv(s.BMM350_Mag_Z),'µT','good'),
            sensorTile('🧭','Hdg',fv(s.BMM350_Heading_Deg,1),'°','good'),
        ].join('');
        $('iDerived').innerHTML=[
            sensorTile('📐','Tilt',fv(s.LSM6_Tilt_Deg,1),'°','good'),
            sensorTile('〰️','Vibration',fv(s.LSM6_Vibration,3),'m/s²',s.LSM6_Vibration>0.5?'warn':'good'),
            sensorTile('⚡','Shock',s.LSM6_Shock_Det?'YES':'No','',s.LSM6_Shock_Det?'danger':'good'),
            sensorTile('🧭','Cardinal',s.BMM350_Cardinal||'--','','good'),
        ].join('');
    });
}

// ═══════════════════════════════════════════════
// SESSION CHARTS — 10-min rolling sparklines
// ═══════════════════════════════════════════════
const CHART_HISTORY = {};
const CHART_MAX = 60; // 60 samples @ ~10s = 10 min
const CHART_DEFS = [
    {key:'SHT45_Temp',      label:'Temperature',   unit:'°C',   color:'#f97316'},
    {key:'METEO_Feels_Like_C',label:'Feels Like',  unit:'°C',   color:'#fb923c'},
    {key:'SHT45_Hum',       label:'Humidity',      unit:'%',    color:'#38bdf8'},
    {key:'SCD41_CO2_ppm',   label:'CO₂',           unit:'ppm',  color:'#a3e635'},
    {key:'BME688_IAQ',      label:'IAQ',           unit:'',     color:'#34d399'},
    {key:'BMV080_PM2_5',    label:'PM2.5',         unit:'µg/m³',color:'#f43f5e'},
    {key:'SGP41_VOC_Index', label:'VOC Index',     unit:'',     color:'#c084fc'},
    {key:'BMP585_Pressure_hPa',label:'Pressure',   unit:'hPa',  color:'#94a3b8'},
    {key:'Geiger_uSvh',     label:'Radiation',     unit:'µSv/h',color:'#facc15'},
    {key:'BMS_State_Of_Charge',label:'Battery',    unit:'%',    color:'#4ade80'},
    {key:'SOLAR_Power_mW',  label:'Solar Power',   unit:'mW',   color:'#fde68a'},
    {key:'LTR390_UVI',      label:'UV Index',      unit:'',     color:'#e879f9'},
];

function loadCharts(){
    const vw=window.innerWidth;
    const w=Math.min(vw-16,800);
    const cards=CHART_DEFS.map(c=>`
        <div class="card">
            <div class="chartbox" style="--chart-h:72px">
                <span class="chart-label">${c.label}</span>
                <canvas id="chart_${c.key}"></canvas>
                <span class="chart-cur" id="cur_${c.key}">--${c.unit}</span>
            </div>
        </div>
    `).join('');
    createWindow('charts-win','Session Charts','📈',100,50,
        `<div style="display:grid;grid-template-columns:1fr 1fr;gap:8px">${cards}</div>
         <div style="font-size:10px;color:var(--text-muted);margin-top:6px;text-align:center">Rolling 10-min session history (current session only)</div>`,
        w);
    pollCharts();
}

function drawSparkline(canvasId, data, color){
    const cvs=$(canvasId); if(!cvs) return;
    const W=cvs.clientWidth||cvs.offsetWidth||200;
    const H=cvs.clientHeight||cvs.offsetHeight||72;
    cvs.width=W; cvs.height=H;
    const ctx=cvs.getContext('2d');
    ctx.clearRect(0,0,W,H);
    if(data.length<2) return;
    const mn=Math.min(...data), mx=Math.max(...data);
    const range=mx-mn||1;
    const pad=4;
    const pts=data.map((v,i)=>[
        pad+(W-2*pad)*(i/(data.length-1)),
        H-pad-(H-2*pad)*((v-mn)/range)
    ]);
    // fill
    const grad=ctx.createLinearGradient(0,0,0,H);
    grad.addColorStop(0,color+'55'); grad.addColorStop(1,color+'00');
    ctx.beginPath(); ctx.moveTo(pts[0][0],H);
    pts.forEach(p=>ctx.lineTo(p[0],p[1]));
    ctx.lineTo(pts[pts.length-1][0],H); ctx.closePath();
    ctx.fillStyle=grad; ctx.fill();
    // line
    ctx.beginPath(); ctx.moveTo(pts[0][0],pts[0][1]);
    pts.forEach(p=>ctx.lineTo(p[0],p[1]));
    ctx.strokeStyle=color; ctx.lineWidth=1.5; ctx.stroke();
    // dot
    const lp=pts[pts.length-1];
    ctx.beginPath(); ctx.arc(lp[0],lp[1],3,0,Math.PI*2);
    ctx.fillStyle=color; ctx.fill();
}

function pollCharts(){
    fetch('/api').then(r=>r.json()).then(d=>{
        const s=d.sensors||{};
        CHART_DEFS.forEach(c=>{
            const v=s[c.key];
            if(v==null) return;
            if(!CHART_HISTORY[c.key]) CHART_HISTORY[c.key]=[];
            CHART_HISTORY[c.key].push(+v);
            if(CHART_HISTORY[c.key].length>CHART_MAX) CHART_HISTORY[c.key].shift();
            const cur=$('cur_'+c.key);
            if(cur) cur.textContent=fv(v)+(c.unit?' '+c.unit:'');
            drawSparkline('chart_'+c.key, CHART_HISTORY[c.key], c.color);
        });
    }).catch(()=>{});
    setTimeout(pollCharts,10000);
}

// ═══════════════════════════════════════════════
// AUTH — password modal for System panel
// ═══════════════════════════════════════════════
const SYS_PASS = 'atlas2025';   // change as needed
let sysUnlocked = false;

function loadSys(){
    if(!sysUnlocked){
        showPassModal(()=>{ sysUnlocked=true; loadSys(); });
        return;
    }
    _buildSysWindow();
}

function showPassModal(onSuccess){
    // Remove any existing modal
    const old=$('pass-modal'); if(old) old.remove();

    const modal = document.createElement('div');
    modal.id='pass-modal';
    modal.style.cssText=`
        position:fixed;inset:0;z-index:20000;
        background:rgba(0,0,0,.65);backdrop-filter:blur(10px);
        display:flex;align-items:center;justify-content:center`;
    modal.innerHTML=`
        <div style="background:rgba(28,28,44,.96);border:1px solid rgba(255,255,255,.12);
                    border-radius:18px;padding:32px 28px;width:320px;max-width:92vw;
                    box-shadow:0 20px 60px rgba(0,0,0,.5);animation:winIn .3s ease">
            <div style="font-size:24px;text-align:center;margin-bottom:6px">🔒</div>
            <div style="font-size:14px;font-weight:600;color:var(--text);text-align:center;margin-bottom:4px">System Access</div>
            <div style="font-size:11px;color:var(--text-muted);text-align:center;margin-bottom:20px">Enter the system password to continue</div>
            <input id="pass-inp" type="password" placeholder="Password…"
                   style="text-align:center;font-size:14px;letter-spacing:.2em;margin-bottom:12px"
                   onkeydown="if(event.key==='Enter')$('pass-ok').click()">
            <div id="pass-err" style="color:var(--danger);font-size:11px;text-align:center;height:16px;margin-bottom:8px"></div>
            <div style="display:flex;gap:8px">
                <button class="btn" style="flex:1" onclick="$('pass-modal').remove()">Cancel</button>
                 <button id="pass-ok" class="btn accent" style="flex:1" onclick="checkPass(SYS_PASS)">Unlock</button>
            </div>
        </div>`;
    document.body.appendChild(modal);
    setTimeout(()=>{ const i=$('pass-inp'); if(i) i.focus(); }, 80);
    // store callback
    window._passCallback = onSuccess;
}

function checkPass(expected){
    const val=($('pass-inp')||{}).value||'';
    if(val===expected){
        $('pass-modal').remove();
        if(window._passCallback) window._passCallback();
    } else {
        const e=$('pass-err');
        if(e){ e.textContent='Incorrect password'; }
        const inp=$('pass-inp');
        if(inp){ inp.value=''; inp.style.borderColor='var(--danger)';
            setTimeout(()=>{ if(inp) inp.style.borderColor=''; },1000); }
    }
}

function lockSys(){
    sysUnlocked=false;
    closeWin('sys-win');
    showToast('System locked');
}

const MODE_BTN_MAP = {
    'Continuous':   {action:'mode_cont',  label:'◉ Continuous',  id:'mbtn_cont'},
    'Light_Sleep':  {action:'mode_light', label:'🌙 Light Sleep', id:'mbtn_light'},
    'Deep_Sleep':   {action:'mode_deep',  label:'💤 Deep Sleep',  id:'mbtn_deep'},
    'Maintenance':  {action:'mode_maint', label:'🔧 Maintenance', id:'mbtn_maint'},
};
function setModeBtn(modeStr){
    Object.values(MODE_BTN_MAP).forEach(m=>{
        const b=$(m.id); if(!b) return;
        b.classList.toggle('mode-active', modeStr===m.action.replace('mode_','').replace('_','_'));
    });
    // normalise: "Continuous" matches 'mode_cont'
    const key=Object.keys(MODE_BTN_MAP).find(k=>modeStr&&modeStr.toLowerCase().replace(' ','_')===k.toLowerCase().replace(' ','_'));
    if(key){ const m=MODE_BTN_MAP[key]; const b=$(m.id); if(b) b.classList.add('mode-active'); }
}
function cmdMode(action){
    cmd(action);
    // Optimistically highlight immediately, then re-confirm from API
    const nm=action.replace('mode_','').replace('cont','Continuous').replace('light','Light_Sleep').replace('deep','Deep_Sleep').replace('maint','Maintenance');
    Object.values(MODE_BTN_MAP).forEach(m=>{ const b=$(m.id); if(b) b.classList.remove('mode-active'); });
    const target=Object.values(MODE_BTN_MAP).find(m=>m.action===action);
    if(target){ const b=$(target.id); if(b) b.classList.add('mode-active'); }
}

function _buildSysWindow(){
    createWindow('sys-win','System (Unlocked)','⚙️',60,60,`
        <div class="card"><div class="card-title">🩺 System Health</div><div id="sHealth">Checking…</div></div>
        <div class="card"><div class="card-title">🎯 Operating Mode</div>
            <div id="sModeNow" style="font-size:11px;color:var(--text-dim);margin-bottom:7px">Current: <b id="sModeName">loading…</b></div>
            <div class="btn-group">
                <button id="mbtn_cont"  class="btn mode-btn" onclick="cmdMode('mode_cont')">◉ Continuous</button>
                <button id="mbtn_light" class="btn mode-btn" onclick="cmdMode('mode_light')">🌙 Light Sleep</button>
                <button id="mbtn_deep"  class="btn mode-btn" onclick="cmdMode('mode_deep')">💤 Deep Sleep</button>
                <button id="mbtn_maint" class="btn mode-btn" onclick="cmdMode('mode_maint')">🔧 Maintenance</button>
            </div>
        </div>
        <div class="card"><div class="card-title">🔧 Hardware Controls</div>
            <div class="btn-group">
                <button class="btn" onclick="openI2CScan()">📡 I2C Scanner</button>
                <button class="btn" onclick="cmd('read_all')">📥 Read Sensors</button>
                <button class="btn" onclick="cmd('init_sensors')">🔄 Re-Init HW</button>
                <button class="btn" onclick="cmd('stop_sensors')">💤 Sleep Sensors</button>
            </div>
        </div>
        <div class="card"><div class="card-title">⚠️ Danger Zone</div>
            <div class="btn-group">
                <button class="btn danger" onclick="cmd('reset_i2c')">⚡ Reset I2C</button>
                <button class="btn danger" onclick="cmd('reboot')">🔁 Reboot</button>
                <button class="btn danger" onclick="cmd('factory_reset')">🗑 Factory Reset</button>
                <button class="btn" onclick="lockSys()" style="margin-left:auto">🔒 Lock</button>
            </div>
        </div>
        <div class="card"><div class="card-title">📦 OTA Firmware Update</div>
            <input type="file" id="otafile" accept=".bin" style="margin-bottom:7px">
            <button class="btn accent" onclick="uploadOTA()" style="width:100%">⬆️ Flash Firmware</button>
            <div id="ota-prog"></div>
        </div>
    `,360);
    fetch('/api').then(r=>r.json()).then(d=>{
        const s=d.sensors||{};
        // Health
        let faults=[],ok=[];
        Object.keys(s).filter(k=>k.startsWith('Fault_')).forEach(k=>{
            const name=k.replace('Fault_','');
            if(s[k]==='ON'||s[k]===true||s[k]==='true') faults.push(`<span style="color:var(--danger);font-size:11px;margin:2px">⚠ ${name}</span>`);
            else ok.push(`<span style="color:var(--accent);font-size:10px;opacity:.6;margin:2px">✓ ${name}</span>`);
        });
        const h=$('sHealth'); if(h) h.innerHTML=(faults.length?faults.join(' '):'<span style="color:var(--accent)">✓ All systems nominal</span>')+'<div style="margin-top:6px">'+ok.join(' ')+'</div>';
        // Mode buttons
        const modeName = s.System_Mode || d.system_mode || '';
        const mn=$('sModeName'); if(mn) mn.textContent=modeName||'unknown';
        // highlight correct button
        const modeKey = modeName.toLowerCase().replace(' ','_');
        Object.keys(MODE_BTN_MAP).forEach(k=>{
            const m=MODE_BTN_MAP[k]; const b=$(m.id); if(!b) return;
            const match=k.toLowerCase().replace(' ','_')===modeKey||
                        m.action.replace('mode_','')===modeKey.replace('_sleep','_sleep');
            b.classList.toggle('mode-active',match);
        });
    });
}

// ═══════════════════════════════════════════════════════════════════
// I2C SCANNER v2 — tooltips, fault dots, EEPROM toggle, discovery
// ═══════════════════════════════════════════════════════════════════

// Full I2C address map (Adafruit + community 2025 list + project specifics)
const I2C_DB = {
    '0x03':{name:'AS3935',desc:'Lightning / EM detector',fault:'Fault_AS3935'},
    '0x0B':{name:'BQ27441',desc:'Fuel gauge (alt address)'},
    '0x10':{name:'VEML7700',desc:'Ambient light sensor',fault:'Fault_VEML7700'},
    '0x14':{name:'BMM350',desc:'Precision 3-axis magnetometer',fault:'Fault_BMM350'},
    '0x18':{name:'LIS2DH',desc:'3-axis accelerometer (alt)'},
    '0x19':{name:'LIS2DH',desc:'3-axis accelerometer'},
    '0x1C':{name:'MMA8452',desc:'3-axis accelerometer'},
    '0x1E':{name:'LIS3MDL / HMC5883',desc:'3-axis magnetometer',fault:'Fault_LIS3MDL'},
    '0x20':{name:'MCP23017',desc:'I/O expander'},
    '0x23':{name:'BH1750',desc:'Digital light sensor'},
    '0x26':{name:'SHT41',desc:'Humidity/Temp (alt)'},
    '0x27':{name:'LCD / SHT21',desc:'LCD backpack or humidity'},
    '0x28':{name:'BNO055',desc:'9-DOF IMU (alt)'},
    '0x29':{name:'TSL2591 / VL53L0X',desc:'Light OR ToF distance',fault:'Fault_TSL2591'},
    '0x2A':{name:'TCS34725 alt',desc:'Color sensor alt addr'},
    '0x29':{name:'TCS34725 / TSL2591',desc:'Color/Light sensor',fault:'Fault_TCS34725'},
    '0x33':{name:'ZMOD4510',desc:'NO2/O3 outdoor air quality',fault:'Fault_ZMOD4510'},
    '0x36':{name:'MAX17048',desc:'LiPo fuel gauge',fault:'Fault_MAX17048'},
    '0x38':{name:'AHT20 / SHT31',desc:'Temperature/humidity sensor'},
    '0x39':{name:'AS7343 / TSL2561',desc:'14-ch spectral / light',fault:'Fault_AS7343'},
    '0x3C':{name:'SSD1306 / SH1106',desc:'0.96" OLED display (128x64)'},
    '0x3D':{name:'SSD1306',desc:'OLED display (alt addr)'},
    '0x40':{name:'INA219 / HTU21D',desc:'Power monitor / humidity',fault:'Fault_INA219'},
    '0x41':{name:'INA219',desc:'Power monitor (alt)'},
    '0x44':{name:'SHT45 / OPT4048',desc:'Humidity+Temp / Color',fault:'Fault_SHT45'},
    '0x45':{name:'SHT30',desc:'Humidity+Temp sensor'},
    '0x47':{name:'BMP585',desc:'Barometric pressure sensor',fault:'Fault_BMP585'},
    '0x48':{name:'ADS1115 / TMP102',desc:'16-bit ADC or temperature'},
    '0x49':{name:'ADS1115',desc:'16-bit ADC (alt)'},
    '0x4A':{name:'ADS1115',desc:'16-bit ADC (alt2)'},
    '0x4B':{name:'ADS1115',desc:'16-bit ADC (alt3)'},
    '0x50':{name:'24Cxx EEPROM',desc:'External EEPROM (BSEC state)',fault:'Fault_I2CMemory'},
    '0x53':{name:'LTR390 / ADXL345',desc:'UV sensor / accelerometer',fault:'Fault_LTR390'},
    '0x57':{name:'BMV080',desc:'Particulate matter PM1/2.5/10',fault:'Fault_BMV080'},
    '0x59':{name:'SGP41',desc:'VOC+NOx gas index sensor',fault:'Fault_SGP41'},
    '0x5A':{name:'CCS811 / MLX90614',desc:'eCO2 VOC / IR thermometer'},
    '0x5B':{name:'CCS811',desc:'eCO2 VOC (alt)'},
    '0x5C':{name:'ILPS22QS / BMP180',desc:'HP pressure+QVAR / pressure',fault:'Fault_ILPS22QS'},
    '0x5D':{name:'ILPS22QS alt',desc:'HP barometric pressure alt'},
    '0x62':{name:'SCD41 / SCD30',desc:'NDIR CO2 sensor',fault:'Fault_SCD41'},
    '0x68':{name:'DS3231 / MPU6050',desc:'RTC or 6-DOF IMU'},
    '0x69':{name:'MPU6050 / ICM-20689',desc:'6-DOF IMU (alt addr)'},
    '0x6A':{name:'LSM6DSOX / L3GD20',desc:'6-DOF IMU (accel+gyro)',fault:'Fault_LSM6DSOX'},
    '0x6B':{name:'LSM6DSOX alt',desc:'6-DOF IMU alt addr'},
    '0x70':{name:'TCA9548A MUX#0',desc:'8-ch I2C multiplexer'},
    '0x71':{name:'TCA9548A MUX#1',desc:'8-ch I2C multiplexer'},
    '0x72':{name:'TCA9548A MUX#2',desc:'8-ch I2C multiplexer'},
    '0x74':{name:'AS7331',desc:'UV-A/B/C spectral sensor',fault:'Fault_AS7331'},
    '0x76':{name:'BME280 / BME680 / MS8607',desc:'Env sensor (addr=0x76)',fault:'Fault_BME280'},
    '0x77':{name:'BME690 / BMP280',desc:'BSEC AI gas sensor',fault:'Fault_BME690'},
    '0x7C':{name:'MCP9808',desc:'Precision temperature sensor'},
};

// Sensors expected on each project MUX/CH for discovery cross-check
const EXPECTED_MAP = [
    {mux:'0x70',ch:0,addr:'0x77',sensor:'BME690'},
    {mux:'0x70',ch:1,addr:'0x6A',sensor:'LSM6DSOX'},
    {mux:'0x70',ch:1,addr:'0x1E',sensor:'LIS3MDL'},
    {mux:'0x70',ch:2,addr:'0x14',sensor:'BMM350'},
    {mux:'0x70',ch:3,addr:'0x50',sensor:'EEPROM'},
    {mux:'0x70',ch:4,addr:'0x36',sensor:'MAX17048'},
    {mux:'0x70',ch:4,addr:'0x40',sensor:'INA219'},
    {mux:'0x70',ch:5,addr:'0x44',sensor:'SHT45'},
    {mux:'0x70',ch:5,addr:'0x47',sensor:'BMP585'},
    {mux:'0x70',ch:6,addr:'0x29',sensor:'TSL2591'},
    {mux:'0x70',ch:6,addr:'0x44',sensor:'OPT4048'},
    {mux:'0x71',ch:0,addr:'0x57',sensor:'BMV080'},
    {mux:'0x71',ch:6,addr:'0x5C',sensor:'ILPS22QS'},
    {mux:'0x72',ch:1,addr:'0x33',sensor:'ZMOD4510'},
    {mux:'0x72',ch:4,addr:'0x62',sensor:'SCD41'},
];

let _i2cFaultData = {};

function openI2CScan(){
    const vw=window.innerWidth, w=Math.min(vw-16,860);
    if($('i2c-win')){ focusWin('i2c-win'); return; }
    createWindow('i2c-win','I2C Bus Scanner','📡',20,40,`
        <div style="display:flex;align-items:center;gap:7px;flex-wrap:wrap;margin-bottom:10px">
            <button class="btn accent" id="scan-btn" onclick="runI2CScan()" style="flex:1;min-width:120px">▶ Scan</button>
            <button class="btn" id="disc-btn" onclick="runDiscovery()" title="Try all known sensor addresses on every MUX channel">🔍 Discovery</button>
            <label style="display:flex;align-items:center;gap:5px;font-size:11px;color:var(--text-dim);cursor:pointer;user-select:none">
                <input type="checkbox" id="excl50" checked style="width:13px;height:13px;accent-color:var(--accent)"> Exclude 0x50 (EEPROM)
            </label>
            <button class="btn" onclick="copyI2CScan()">📋 JSON</button>
            <span id="scan-status" style="font-size:10px;color:var(--text-dim);white-space:nowrap"></span>
        </div>
        <!-- Legend -->
        <div style="display:flex;gap:12px;flex-wrap:wrap;margin-bottom:10px;font-size:10px;color:var(--text-dim)">
            <span><span class="i2c-dot ok" style="display:inline-block;vertical-align:middle;margin-right:3px"></span>OK & assigned</span>
            <span><span class="i2c-dot bad" style="display:inline-block;vertical-align:middle;margin-right:3px"></span>Fault / error</span>
            <span><span class="i2c-dot unknown" style="display:inline-block;vertical-align:middle;margin-right:3px"></span>Present, unassigned</span>
            <span><span class="i2c-dot unassigned" style="display:inline-block;vertical-align:middle;margin-right:3px"></span>In DB, not seen</span>
            <span>❓ hover for info</span>
        </div>
        <div id="i2c-result" style="font-size:11px;min-height:200px"></div>
        <div id="disc-result" style="font-size:11px;margin-top:10px"></div>
    `,w);
    // Load fault data then scan
    fetch('/api').then(r=>r.json()).then(d=>{ _i2cFaultData=d.sensors||{}; }).catch(()=>{});
    runI2CScan();
}

function i2cAddrHTML(a, muxCol){
    const excl = ($('excl50')||{}).checked!==false;
    if(excl && a==='0x50') return '';
    const db = I2C_DB[a];
    const fk = db && db.fault;
    const fv = fk ? _i2cFaultData[fk] : undefined;
    let dotCls='unknown', statusText='Present, not mapped', statusCol='#f59e0b';
    if(db){
        if(fk !== undefined){
            const isBad = (String(fv)==='true'||String(fv)==='1'||String(fv)==='ON');
            dotCls=isBad?'bad':'ok';
            statusText=isBad?'FAULT DETECTED':'Sensor OK';
            statusCol=isBad?'#ef4444':'#22c55e';
        } else {
            dotCls='ok'; statusText='Recognised, no fault key'; statusCol='#94a3b8';
        }
    }
    const name=db?db.name:'Unknown device';
    const desc=db?db.desc:'Not in database';
    return `<span class="i2c-addr">
        <span class="i2c-dot ${dotCls}"></span>
        <span style="color:${muxCol};font-family:'JetBrains Mono',monospace">${a}</span>
        ${db?`<span style="color:var(--text-dim);font-size:9px;max-width:70px;overflow:hidden;text-overflow:ellipsis"> ${db.name}</span>`:''}
        <span class="i2c-qmark">?
            <div class="i2c-tt">
                <div class="i2c-tt-name">${name}</div>
                <div class="i2c-tt-addr">${a}</div>
                <div class="i2c-tt-status" style="color:${statusCol}">${statusText}</div>
                <div class="i2c-tt-desc">${desc}</div>
            </div>
        </span>
    </span>`;
}

function runI2CScan(){
    const btn=$('scan-btn'), st=$('scan-status'), res=$('i2c-result');
    if(!res) return;
    if(btn){ btn.disabled=true; btn.textContent='⏳ Scanning…'; }
    if(st) st.textContent='';
    res.innerHTML=`<div style="color:var(--text-dim);padding:12px 0">⏳ Scanning all MUX channels…</div>`;
    fetch('/api').then(r=>r.json()).then(d=>{ _i2cFaultData=d.sensors||{}; }).catch(()=>{});
    const t0=Date.now();
    fetch('/scan_json').then(r=>r.json()).then(data=>{
        if(st) st.textContent=`Done in ${Date.now()-t0}ms`;
        if(btn){ btn.disabled=false; btn.textContent='▶ Scan'; }
        renderI2CScan(data);
    }).catch(err=>{
        if(btn){ btn.disabled=false; btn.textContent='▶ Scan'; }
        if(res) res.innerHTML=`<div style="color:var(--danger)">✗ ${err}</div>`;
    });
}

function renderI2CScan(data){
    const res=$('i2c-result'); if(!res) return;
    const mux_colors=['#00d4aa','#a78bfa','#f59e0b'];
    let html='';

    if(data.muxes) data.muxes.forEach((mux,mi)=>{
        const col=mux_colors[mi]||'#888';
        html+=`<div style="margin-bottom:14px">
            <div style="font-weight:700;color:${col};margin-bottom:7px;font-size:12px;
                border-bottom:1px solid rgba(255,255,255,.07);padding-bottom:4px">
                ■ MUX ${mux.addr}</div>`;
        if(mux.channels) mux.channels.forEach(ch=>{
            const devs=ch.devices||[], offline=ch.status==='OFFLINE';
            html+=`<div style="display:flex;align-items:center;gap:6px;padding:4px 0;
                              border-bottom:1px solid rgba(255,255,255,.04)">
                <span style="color:${offline?'var(--danger)':'var(--text-muted)'};
                    min-width:46px;font-size:10px;font-family:'JetBrains Mono',monospace">CH${ch.ch}</span>
                <span style="flex:1;display:flex;flex-wrap:wrap;gap:2px">`;
            if(offline){
                html+=`<span style="color:var(--danger);font-size:10px">OFFLINE</span>`;
            } else if(devs.length===0){
                html+=`<span style="color:var(--text-muted);font-style:italic;font-size:10px">empty</span>`;
            } else {
                html+=devs.map(a=>i2cAddrHTML(a,col)).join('');
            }
            html+=`</span></div>`;
        });
        html+=`</div>`;
    });

    if(data.main_bus){
        const mb=data.main_bus.devices||[];
        html+=`<div style="margin-top:8px;padding-top:8px;border-top:1px solid rgba(255,255,255,.08)">
            <div style="font-weight:700;color:#94a3b8;margin-bottom:7px;font-size:12px">■ Main Bus (direct)</div>
            <div style="display:flex;flex-wrap:wrap;gap:3px">
            ${mb.length?mb.map(a=>i2cAddrHTML(a,'#94a3b8')).join(''):`<span style="color:var(--text-muted);font-style:italic;font-size:11px">No devices</span>`}
            </div></div>`;
    }

    res.innerHTML=html;
    res.dataset.raw=JSON.stringify(data,null,2);
}

// ── Discovery scan: try all known addresses on every MUX/CH ──
async function runDiscovery(){
    const btn=$('disc-btn'), res=$('disc-result');
    if(!res) return;
    if(btn){ btn.disabled=true; btn.textContent='⏳ Discovering…'; }
    res.innerHTML=`<div style="border-top:1px solid rgba(255,255,255,.1);padding-top:10px;margin-top:4px">
        <div style="font-size:12px;font-weight:700;color:#f59e0b;margin-bottom:8px">🔍 Discovery Scan</div>
        <div style="color:var(--text-dim);font-size:11px">Querying /scan_json for all addresses…</div></div>`;

    let scanData;
    try { scanData = await fetch('/scan_json').then(r=>r.json()); }
    catch(e){ res.innerHTML=`<div style="color:var(--danger)">Discovery failed: ${e}</div>`; if(btn){btn.disabled=false;btn.textContent='🔍 Discovery';} return; }

    // Build flat map: mux+ch -> [addr]
    const found = {}; // key=addr -> [{mux,ch}]
    if(scanData.muxes) scanData.muxes.forEach(mux=>{
        if(mux.channels) mux.channels.forEach(ch=>{
            (ch.devices||[]).forEach(a=>{
                if(!found[a]) found[a]=[];
                found[a].push({mux:mux.addr, ch:ch.ch});
            });
        });
    });

    // Compare against expected map
    let html=`<div style="border-top:1px solid rgba(255,255,255,.1);padding-top:10px;margin-top:4px">
        <div style="font-size:12px;font-weight:700;color:#f59e0b;margin-bottom:8px">🔍 Discovery Report</div>`;

    let issues=0;
    EXPECTED_MAP.forEach(exp=>{
        const locs=found[exp.addr]||[];
        const onExpected=locs.some(l=>l.mux===exp.mux&&l.ch===exp.ch);
        const onOther=locs.filter(l=>!(l.mux===exp.mux&&l.ch===exp.ch));
        if(!onExpected){
            issues++;
            if(onOther.length){
                html+=`<div class="disc-item disc-found">
                    <span style="color:#f59e0b;font-weight:700">⚠ ${exp.sensor}</span>
                    <span style="color:var(--text-dim)"> expected at MUX ${exp.mux} CH${exp.ch} — </span>
                    <span style="color:#22c55e">found at: ${onOther.map(l=>`MUX ${l.mux} CH${l.ch}`).join(', ')}</span>
                    <div style="color:var(--text-muted);font-size:10px;margin-top:2px">Address ${exp.addr} (${I2C_DB[exp.addr]?I2C_DB[exp.addr].desc:'?'})</div>
                </div>`;
            } else {
                html+=`<div class="disc-item disc-miss">
                    <span style="color:#ef4444;font-weight:700">✗ ${exp.sensor}</span>
                    <span style="color:var(--text-dim)"> not found anywhere</span>
                    <div style="color:var(--text-muted);font-size:10px;margin-top:2px">Expected: MUX ${exp.mux} CH${exp.ch} @ ${exp.addr}</div>
                </div>`;
            }
        }
    });

    // Find unexpected addresses (not in EXPECTED_MAP)
    const expectedAddrs=EXPECTED_MAP.map(e=>e.addr);
    const muxAddrs=['0x70','0x71','0x72'];
    let unexpected=[];
    Object.keys(found).forEach(addr=>{
        if(muxAddrs.includes(addr)||addr==='0x50') return;
        if(!expectedAddrs.includes(addr)){
            found[addr].forEach(loc=>{
                const db=I2C_DB[addr];
                unexpected.push({addr,loc,name:db?db.name:'Unknown',desc:db?db.desc:'Not in database'});
            });
        }
    });
    if(unexpected.length){
        html+=`<div style="margin-top:8px;font-size:11px;font-weight:700;color:#a78bfa">🆕 Unexpected / unassigned devices:</div>`;
        unexpected.forEach(u=>{
            html+=`<div class="disc-item" style="border-left:3px solid #a78bfa;margin-top:4px">
                <span style="color:#a78bfa;font-weight:700">${u.addr}</span>
                <span style="color:var(--text-dim)"> @ MUX ${u.loc.mux} CH${u.loc.ch}</span>
                <span style="color:var(--text);margin-left:6px">${u.name}</span>
                <div style="color:var(--text-muted);font-size:10px;margin-top:2px">${u.desc}</div>
            </div>`;
        });
    }

    if(!issues && !unexpected.length){
        html+=`<div style="color:#22c55e;font-size:11px;padding:8px 0">✓ All expected sensors found on correct MUX/CH. No anomalies detected.</div>`;
    }

    html+=`<div style="font-size:10px;color:var(--text-muted);margin-top:8px;text-align:right">${Object.values(found).flat().length} device-slots found · ${EXPECTED_MAP.length} expected</div></div>`;
    res.innerHTML=html;
    if(btn){ btn.disabled=false; btn.textContent='🔍 Discovery'; }
}

function copyI2CScan(){
    const res=$('i2c-result'); if(!res) return;
    const raw=res.dataset.raw;
    if(raw) navigator.clipboard?.writeText(raw).then(()=>showToast('Scan copied (JSON)'));
}

// ═══════════════════════════════════════════════
// LOGS — wide, scrollable, copy on select
// ═══════════════════════════════════════════════
let logsRunning=false;
function loadLogs(){
    const vw=window.innerWidth, w=Math.min(vw-16,900);
    createWindow('log-win','Live Log Stream','📜',30,40,`
        <div class="log-toolbar">
            <input class="log-search" id="logSearch" placeholder="🔍 Filter log…" oninput="filterLog()">
            <button class="btn accent" onclick="clearLog()">Clear</button>
            <button class="btn" onclick="scrollLogBottom()">⬇ End</button>
            <button class="btn" onclick="pasteLog()">📋 Copy All</button>
        </div>
        <div id="log-terminal" onmouseup="autoSelectCopy()"></div>
    `,w);
    logsRunning=true;
    pollLogs();
}

let _lastLogText='';
function pollLogs(){
    if(!$('log-terminal')){ logsRunning=false; return; }
    fetch('/logs').then(r=>r.text()).then(t=>{
        const term=$('log-terminal'); if(!term) return;
        if(t!==_lastLogText){
            _lastLogText=t;
            const q=($('logSearch')||{}).value||'';
            renderLog(t,q);
        }
    }).catch(()=>{});
    if(logsRunning) setTimeout(pollLogs,1500);
}

function renderLog(txt,q){
    const term=$('log-terminal'); if(!term) return;
    const wantScroll=(term.scrollHeight-term.scrollTop-term.clientHeight)<60;
    if(q){
        const re=new RegExp(q.replace(/[.*+?^${}()|[\]\\]/g,'\\$&'),'gi');
        term.innerHTML=txt.replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;')
            .replace(re,m=>`<mark style="background:rgba(0,212,170,.3);color:#fff;border-radius:2px">${m}</mark>`);
    } else {
        term.textContent=txt;
    }
    if(wantScroll) term.scrollTop=term.scrollHeight;
}
function filterLog(){ if(_lastLogText) renderLog(_lastLogText,($('logSearch')||{}).value||''); }
function clearLog(){ const t=$('log-terminal'); if(t){t.textContent=''; _lastLogText='';} }
function scrollLogBottom(){ const t=$('log-terminal'); if(t) t.scrollTop=t.scrollHeight; }
function pasteLog(){
    const t=$('log-terminal'); if(!t) return;
    navigator.clipboard?.writeText(t.innerText||t.textContent).then(()=>showToast('Log copied!'));
}
function autoSelectCopy(){
    const sel=window.getSelection();
    if(sel&&sel.toString().length>0){
        navigator.clipboard?.writeText(sel.toString()).then(()=>showToast('Copied selection'));
    }
}

function uploadOTA(){
    const f=$('otafile'); if(!f||!f.files[0]) return notify('Select .bin first',false);
    const fd=new FormData(); fd.append('update',f.files[0]);
    const xhr=new XMLHttpRequest(); xhr.open('POST','/update',true);
    xhr.upload.onprogress=e=>{if(e.lengthComputable) $('ota-prog').textContent=(e.loaded/e.total*100).toFixed(1)+'%';};
    xhr.onload=()=>{$('ota-prog').textContent=xhr.status===200?'✓ SUCCESS — rebooting…':'✗ FAILED';};
    xhr.send(fd);
}

// ═══════════════════════════════════════════════════════════════════
// 🏥 MEDICAL DASHBOARD – human-readable, risk-oriented
// ═══════════════════════════════════════════════════════════════════
// ── Risk card (hero version for Medical panel) ──
function riskCard(icon,label,val,unit,cat,warnAt,dangerAt,maxVal,desc){
    const n = (val!=null&&val!==undefined) ? parseFloat(val) : null;
    const safe = n!=null && Number.isFinite(n);
    const st = safe ? (n>=dangerAt?'danger':n>=warnAt?'warn':'good') : 'good';
    const col = st==='danger'?'#ef4444':st==='warn'?'#f59e0b':'#22c55e';
    const bg  = st==='danger'?'rgba(239,68,68,.07)':st==='warn'?'rgba(245,158,11,.07)':'rgba(34,197,94,.05)';
    const pct = safe ? Math.min(100,(n/maxVal)*100) : 0;
    const fillGrad = pct>70
        ? 'linear-gradient(90deg,#22c55e,#f59e0b 40%,#ef4444)'
        : pct>40 ? 'linear-gradient(90deg,#22c55e,#f59e0b)'
        : 'linear-gradient(90deg,#22c55e,#4ade80)';
    const warnPct = Math.min(100,(warnAt/maxVal)*100);
    const danPct  = Math.min(100,(dangerAt/maxVal)*100);
    return `<div style="background:${bg};border-radius:12px;padding:14px 15px;
                        border:1px solid rgba(255,255,255,.07);border-left:3px solid ${col};
                        margin-bottom:8px">
        <div style="display:flex;align-items:center;gap:10px;margin-bottom:8px">
            <span style="font-size:20px">${icon}</span>
            <div style="flex:1">
                <div style="font-size:9px;color:var(--text-muted);text-transform:uppercase;
                    letter-spacing:.7px;font-weight:700">${label}</div>
                ${desc?`<div style="font-size:9px;color:var(--text-muted);opacity:.7;margin-top:1px">${desc}</div>`:''}
            </div>
            <div style="text-align:right">
                <div style="font-size:24px;font-weight:800;color:${col};line-height:1;
                    font-family:'JetBrains Mono',monospace">${safe?fv(n,n<10?1:0):'--'}
                    <span style="font-size:11px;font-weight:400;color:var(--text-dim)">${unit}</span>
                </div>
                ${cat?`<div style="font-size:10px;color:${col};font-weight:600;margin-top:2px">${cat}</div>`:''}
            </div>
        </div>
        <div style="position:relative;height:6px">
            <div style="width:100%;height:100%;border-radius:999px;background:rgba(255,255,255,.07)"></div>
            <div style="position:absolute;left:0;top:0;height:100%;border-radius:999px;
                background:${fillGrad};width:${pct}%;transition:width .7s cubic-bezier(.4,0,.2,1);
                box-shadow:0 0 8px ${col}55"></div>
            <div style="position:absolute;top:-2px;bottom:-2px;width:2px;border-radius:1px;
                left:${warnPct}%;background:rgba(245,158,11,.8)"></div>
            <div style="position:absolute;top:-2px;bottom:-2px;width:2px;border-radius:1px;
                left:${danPct}%;background:rgba(239,68,68,.9)"></div>
        </div>
    </div>`;
}

function loadMedical(){
    const vw=window.innerWidth, w=Math.min(vw-16,520);
    createWindow('med-win','🏥 '+t('medical'),'🏥',20,40,`
        <div id="medGrid" style="color:var(--text-muted);font-size:11px;text-align:center;padding:20px">
            ⏳ Loading…
        </div>
    `,w);
    fetch('/api').then(r=>r.json()).then(d=>{
        const s=d.sensors||{};
        const g=$('medGrid'); if(!g) return;

        // Biometeo alert banner
        const ba=s.MED_Biometeo_Alert;
        let html='';
        if(ba==='HIGH'||ba==='MODERATE'){
            const c=ba==='HIGH'?'#ef4444':'#f59e0b';
            html+=`<div style="background:${c}18;border:1px solid ${c}44;border-radius:10px;
                padding:12px 14px;margin-bottom:12px;display:flex;align-items:center;gap:10px">
                <span style="font-size:22px">${ba==='HIGH'?'🚨':'⚠️'}</span>
                <div>
                    <div style="font-size:12px;font-weight:700;color:${c}">${t('biometeo')}: ${ba}</div>
                    <div style="font-size:10px;color:var(--text-dim)">Score: ${s.MED_Biometeo_Score!=null?(+s.MED_Biometeo_Score).toFixed(1)+'/10':''}</div>
                </div>
            </div>`;
        }

        // Section: Pain & Pressure Sensitivity
        html+=`<div style="font-size:10px;font-weight:700;color:var(--text-muted);text-transform:uppercase;
            letter-spacing:.7px;margin:0 0 8px;padding-bottom:4px;border-bottom:1px solid rgba(255,255,255,.07)">
            🩺 Pain & Pressure Sensitivity</div>`;
        html+=riskCard('🧠',t('migraine'),s.MED_Migraine_Risk,'/10',s.MED_Migraine_Cat,5,7,10,'Barometric + humidity driver');
        html+=riskCard('🦴',t('rheum'),s.MED_Rheumatic_Risk,'/10',s.MED_Rheumatic_Cat,5,7,10,'Joint & tissue pressure sensitivity');
        html+=riskCard('📊',t('bpi'),s.MED_Baro_Pain_Index,'/10',null,6,8,10,'Shutty chronic pain index');
        html+=riskCard('👃',t('sinus'),s.MED_Sinus_Risk,'/10',null,4,7,10,'Pressure delta + humidity');

        // Section: Respiratory
        html+=`<div style="font-size:10px;font-weight:700;color:var(--text-muted);text-transform:uppercase;
            letter-spacing:.7px;margin:12px 0 8px;padding-bottom:4px;border-bottom:1px solid rgba(255,255,255,.07)">
            🫁 Respiratory & Air Quality</div>`;
        html+=riskCard('🫁',t('lung')+' Rest',s.MED_Lung_Deposit_Rest,'µg/min',null,0.3,0.8,2,'ICRP alveolar deposition model');
        html+=riskCard('🫁',t('lung')+' Exercise',s.MED_Lung_Deposit_Exer,'µg/min',null,0.8,2.0,5,'During physical activity');
        html+=riskCard('🌿','IAQ Score',s.GAS_IAQ_Score,'/100',s.GAS_Toxicity_Name,
            s.GAS_IAQ_Score>50?50:s.GAS_IAQ_Score,25,100,'BSEC AI indoor air quality');
        html+=riskCard('💨','Gas Toxicity',s.GAS_Toxicity_Risk,'/10',s.GAS_Toxicity_Name,5,7,10,'Pattern-based gas hazard');
        html+=riskCard('💧','Required ACH',s.MED_Required_ACH,'/h',null,3,8,12,'Air changes/h needed for CO₂ dilution');

        // Section: Radiation & Environment
        html+=`<div style="font-size:10px;font-weight:700;color:var(--text-muted);text-transform:uppercase;
            letter-spacing:.7px;margin:12px 0 8px;padding-bottom:4px;border-bottom:1px solid rgba(255,255,255,.07)">
            ☢️ Radiation & Thermal</div>`;
        html+=riskCard('☢️','Radiation Dose',s.Geiger_uSvh,'µSv/h',null,0.2,0.5,1.0,'Background / natural limit');
        html+=riskCard('🌡️',t('utci'),s.MED_UTCI_C,'°C',s.MED_UTCI_Cat,-5,32,50,'Universal Thermal Climate Index');
        html+=riskCard('🏠',t('radon'),s.MED_Radon_Risk,'/10',null,5,7.5,10,'Radon accumulation proxy');

        g.style.display='block'; g.innerHTML=html;
    }).catch(()=>{ const g=$('medGrid'); if(g) g.innerHTML='<div style="color:var(--danger)">Connection error</div>'; });
}

// ═══════════════════════════════════════════════════════════════════
// 🌌 SPACE WEATHER DASHBOARD
// ═══════════════════════════════════════════════════════════════════
function loadSpaceWeather(){
    const vw=window.innerWidth, w=Math.min(vw-16,520);
    createWindow('space-win','🌌 '+t('space'),'🌌',40,40,`
        <div id="spaceGrid"><div style="color:var(--text-muted);padding:20px;text-align:center">⏳ Loading…</div></div>
    `,w);
    fetch('/api').then(r=>r.json()).then(d=>{
        const s=d.sensors||{};
        const g=$('spaceGrid'); if(!g) return;

        const k=+s.SPACE_K_Index_Proxy||0;
        const kcolor = k>=7?'#ff4757':k>=5?'#ffa502':k>=3?'#f59e0b':'#4ade80';
        const fb=+s.SPACE_Forbush_Pct||0;
        const aur=+s.SPACE_Aurora_Prob_Pct||0;
        const dbdt=+s.SPACE_dBdt_nTs||0;

        let html='';

        // K-index big display
        html+=`<div style="background:${kcolor}18;border:2px solid ${kcolor}55;border-radius:14px;
                           padding:18px;text-align:center;margin-bottom:10px">
            <div style="font-size:10px;color:var(--text-muted);text-transform:uppercase;letter-spacing:.5px">Local K-Index (proxy)</div>
            <div style="font-size:56px;font-weight:800;color:${kcolor};line-height:1;margin:6px 0">${k>=0?k:'?'}</div>
            <div style="font-size:12px;color:${kcolor}">${s.SPACE_SSC_Class||'Quiet'}</div>
            <div style="font-size:10px;color:var(--text-muted);margin-top:4px">dB/dt: ${fv(s.SPACE_dBdt_nTs,2)} nT/s &nbsp;|&nbsp; Dev: ${fvi(s.SPACE_B_Dev_nT)} nT</div>
        </div>`;

        // Aurora
        const aurColor = aur>50?'#818cf8':aur>10?'#a78bfa':'#4ade80';
        html+=`<div style="background:rgba(0,0,0,.22);border-radius:12px;padding:14px;margin-bottom:8px">
            <div style="display:flex;align-items:center;justify-content:space-between;margin-bottom:8px">
                <div style="font-size:12px;font-weight:600;color:var(--text)">🌌 ${t('aurora')}</div>
                <div style="font-size:22px;font-weight:800;color:${aurColor}">${fv(s.SPACE_Aurora_Prob_Pct,1)}%</div>
            </div>
            <div style="background:rgba(255,255,255,.07);border-radius:4px;height:8px;overflow:hidden;margin-bottom:8px">
                <div style="width:${Math.min(100,aur)}%;height:100%;background:${aurColor};border-radius:4px;transition:width .5s"></div>
            </div>
            <div style="font-size:10px;color:var(--text-muted)">
                Sky: <span style="color:var(--text)">${s.MLX_Sky_Condition||'Unknown'}</span> &nbsp;|&nbsp;
                Clear window: <span style="color:${s.SPACE_Aurora_Sky_Clear==='YES'?'var(--accent)':'var(--danger)'}">${s.SPACE_Aurora_Sky_Clear||'?'}</span>
            </div>
        </div>`;

        // Forbush
        const fbColor = fb<-8?'#ff4757':fb<-5?'#ffa502':fb<-2?'#f59e0b':'#4ade80';
        html+=bigCard('☢️',t('forbush'),fv(s.SPACE_Forbush_Pct,1)+'%',
            s.SPACE_Forbush_Class||'Normal',null,fbColor,fb<-5?'danger':fb<-2?'warn':'good');

        // Ozone
        html+=bigCard('🔵',t('ozone'),fvi(s.SPACE_Ozone_DU||s.SPACE_Ozone_DU_Proxy),'DU',
            'Stratospheric proxy','#38bdf8','good');

        // GCR Radiation
        html+=bigCard('⚡','Cosmic Ray Flux',fv(s.Geiger_CPM_Corrected,0),'CPM (corrected)',
            `GCR: ${fv(s.Geiger_uSvh,3)} µSv/h`,'#facc15',
            +s.Geiger_uSvh>0.5?'danger':+s.Geiger_uSvh>0.2?'warn':'good');

        // Magnetic field
        html+=`<div style="background:rgba(0,0,0,.22);border-radius:12px;padding:14px;margin-bottom:8px">
            <div style="font-size:10px;color:var(--text-muted);text-transform:uppercase;letter-spacing:.5px;margin-bottom:8px">🧲 Magnetic Field (BMM350)</div>
            <div style="display:grid;grid-template-columns:1fr 1fr 1fr;gap:6px;text-align:center">
                ${['X','Y','Z'].map((ax,i)=>{
                    const v=[s.BMM350_Mag_X,s.BMM350_Mag_Y,s.BMM350_Mag_Z][i];
                    return `<div style="background:rgba(255,255,255,.05);border-radius:8px;padding:8px">
                        <div style="font-size:9px;color:var(--text-muted)">${ax} [µT]</div>
                        <div style="font-size:16px;font-weight:700;color:#818cf8">${fv(v,2)}</div>
                    </div>`;
                }).join('')}
            </div>
            <div style="text-align:center;margin-top:8px;font-size:13px;color:var(--text)">
                |B| = <strong style="color:#818cf8">${fv(s.SPACE_B_Total_uT,2)} µT</strong> &nbsp;
                Heading: <strong>${fv(s.BMM350_Heading_Deg,1)}° ${s.BMM350_Cardinal||''}</strong>
            </div>
        </div>`;

        // Solar AOD
        html+=bigCard('🌞',t('aod'),fv(s.ASTRO_AOD,3),'',
            'Aerosol Optical Depth','#fbbf24',+s.ASTRO_AOD>0.4?'warn':'good');

        g.innerHTML=html;
    }).catch(()=>{ const g=$('spaceGrid'); if(g) g.innerHTML='<div style="color:var(--danger)">Error</div>'; });
}

// ═══════════════════════════════════════════════════════════════════
// 🌿 ENVIRONMENTAL DASHBOARD
// ═══════════════════════════════════════════════════════════════════
function loadEnviron(){
    const vw=window.innerWidth, w=Math.min(vw-16,560);
    createWindow('env2-win','🌿 '+t('environ'),'🌿',50,40,`
        <div id="envGrid"><div style="color:var(--text-muted);padding:20px;text-align:center">⏳</div></div>
    `,w);
    fetch('/api').then(r=>r.json()).then(d=>{
        const s=d.sensors||{};
        const g=$('envGrid'); if(!g) return;
        let html='';

        // Weather headline
        html+=`<div style="background:linear-gradient(135deg,rgba(0,212,170,.12),rgba(56,189,248,.08));
                           border:1px solid rgba(0,212,170,.2);border-radius:14px;padding:16px;margin-bottom:10px">
            <div style="display:flex;justify-content:space-between;align-items:flex-start">
                <div>
                    <div style="font-size:36px;font-weight:800;color:var(--accent)">${fv(s.SHT45_Temp,1)}°C</div>
                    <div style="font-size:13px;color:var(--text-dim)">Feels ${fv(s.METEO_Feels_Like_C,1)}°C &nbsp; ${t('hum')}: ${fv(s.SHT45_Hum,0)}%</div>
                </div>
                <div style="text-align:right">
                    <div style="font-size:14px;color:var(--text-dim)">${s.MLX_Sky_Condition||'--'}</div>
                    <div style="font-size:11px;color:var(--text-muted)">${fv(s.METEO_Sea_Level_Press_hPa,1)} hPa (SLP)</div>
                    <div style="font-size:11px;color:var(--text-muted)">Zambretti: ${s.METEO_Zambretti_Forecast||'--'}</div>
                </div>
            </div>
        </div>`;

        // Air quality section
        const eaqi=+s.AIR_EAQI_Index||0;
        const eaqiCol=eaqi>=5?'#ff4757':eaqi>=4?'#f97316':eaqi>=3?'#ffa502':eaqi>=2?'#facc15':'#4ade80';
        html+=`<div style="background:rgba(0,0,0,.22);border-radius:12px;padding:14px;margin-bottom:8px">
            <div style="display:flex;align-items:center;gap:12px">
                <div style="background:${eaqiCol}22;border-radius:50%;width:52px;height:52px;display:flex;align-items:center;
                            justify-content:center;font-size:22px;font-weight:800;color:${eaqiCol};flex-shrink:0">${eaqi}</div>
                <div>
                    <div style="font-size:12px;font-weight:600;color:var(--text)">${s.AIR_Quality_Status||'Unknown'}</div>
                    <div style="font-size:10px;color:var(--text-muted)">EAQI &nbsp;|&nbsp; WHO: ${fv(s.AIR_WHO_AQI_Pct,0)}% &nbsp;|&nbsp; IAQ: ${fvi(s.BME688_IAQ)}</div>
                    <div style="font-size:10px;color:var(--text-muted)">Visibility: ${fv(s.AIR_Visibility_Km,1)} km &nbsp;|&nbsp; Smog: ${fv(s.AIR_Smog_Index,1)}</div>
                </div>
            </div>
        </div>`;

        // PM + Gas grid
        html+=`<div style="display:grid;grid-template-columns:1fr 1fr;gap:8px;margin-bottom:8px">`;
        [
            ['🌫️','PM1.0',s.BMV080_PM1_0,'µg/m³',0],
            ['🌫️','PM2.5',s.BMV080_PM2_5,'µg/m³',35],
            ['🌫️','PM10',s.BMV080_PM10_0,'µg/m³',50],
            ['🫧','CO₂ (NDIR)',s.SCD41_CO2_ppm,'ppm',1000],
            ['💨','VOC Index',s.SGP41_VOC_Index,'',150],
            ['🔬','NOx Index',s.SGP41_NOx_Index,'',20],
            ['🧪','NO2',s.ZMOD4510_NO2_ppb,'ppb',50],
            ['🌞','O3',s.ZMOD4510_O3_ppb,'ppb',70],
        ].forEach(([ic,lb,val,un,thr])=>{
            if(val==null) return;
            const n=+val; const col=n>thr?'#ffa502':'#4ade80';
            html+=`<div style="background:rgba(0,0,0,.22);border-radius:10px;padding:10px">
                <div style="font-size:9px;color:var(--text-muted)">${ic} ${lb}</div>
                <div style="font-size:20px;font-weight:700;color:${col}">${fv(val,1)}</div>
                <div style="font-size:9px;color:var(--text-dim)">${un}</div>
            </div>`;
        });
        html+=`</div>`;

        // Weather details
        html+=`<div style="background:rgba(0,0,0,.22);border-radius:12px;padding:14px;margin-bottom:8px">
            <div style="font-size:10px;color:var(--text-muted);text-transform:uppercase;letter-spacing:.5px;margin-bottom:8px">🌤 Meteorological Derived</div>
            <div style="display:grid;grid-template-columns:1fr 1fr;gap:6px;font-size:11px">
                ${[
                    ['Dew Point',s.METEO_Dew_Point_C,'°C'],
                    ['Wet Bulb',s.METEO_Wet_Bulb_C,'°C'],
                    ['Cloud Base',s.METEO_Cloud_Base_m,'m'],
                    ['LCL Alt.',s.METEO_LCL_m,'m'],
                    ['Air Density',s.METEO_Air_Density,'kg/m³'],
                    ['θe Instab.',s.METEO_ThetaE_K,'K'],
                    ['Conv. Instab.',s.METEO_Conv_Instability,'/10'],
                    ['GHI Solar',s.METEO_Solar_GHI_Wm2,'W/m²'],
                    ['Precipitable H₂O',s.METEO_PW_mm,'mm'],
                    ['Frost Risk',s.METEO_Frost_Risk_Pct,'%'],
                    ['Zambretti',s.METEO_Zambretti_Forecast,''],
                    ['Fire Risk',s.ENV_Fire_Risk_Pct,'%'],
                ].map(([lb,val,un])=>val!=null?
                    `<div style="background:rgba(255,255,255,.04);border-radius:6px;padding:6px 8px">
                        <div style="color:var(--text-muted);font-size:9px">${lb}</div>
                        <div style="color:var(--text);font-weight:600">${fv(val,1)} <span style="color:var(--text-dim);font-size:9px">${un}</span></div>
                    </div>`:'').join('')}
            </div>
        </div>`;

        // Wind (from WU)
        if(s.WIND_Speed_Kph!=null){
            const dir=+s.WIND_Direction_Deg||0;
            html+=bigCard('💨','Wind',fv(s.WIND_Speed_Kph,1)+' km/h',
                `→ ${s.WIND_Speed_Kph}°`, `Gust: ${fv(s.WIND_Gust_Kph,1)} km/h`,'#38bdf8','good');
        }

        // UV
        const uvi=+s.LTR390_UVI||0;
        html+=bigCard('☀️',t('uvi'),fv(s.LTR390_UVI,1),'',
            `${getUVName(uvi)} | VD: ${fv(s.MED_VitD_Time_min,0)} min`,'#f59e0b',
            uvi>8?'danger':uvi>3?'warn':'good');

        g.innerHTML=html;
    });
}
function getUVName(v){ return v<3?'Low':v<6?'Moderate':v<8?'High':v<11?'Very High':'Extreme'; }

// ═══════════════════════════════════════════════════════════════════
// 🌈 OPTICAL SPECTROMETER
// ═══════════════════════════════════════════════════════════════════
function loadSpectrometer(){
    const vw=window.innerWidth, w=Math.min(vw-16,540);
    createWindow('spec-win','🌈 Optical Spectrometer','🌈',30,40,`
        <div style="font-size:10px;color:var(--text-muted);text-align:center;margin-bottom:8px">
            Spectral & UV Analysis &nbsp;|&nbsp; AS7343 (14-ch) + AS7331 (UVA/B/C)
        </div>
        <div style="background:rgba(0,0,0,.3);border-radius:10px;padding:12px;margin-bottom:8px">
            <canvas id="spec-canvas" style="width:100%;height:120px;display:block"></canvas>
            <div id="spec-gain" style="text-align:right;font-size:10px;color:var(--text-muted);margin-top:4px"></div>
        </div>
        <div id="spec-uv" style="display:grid;grid-template-columns:1fr 1fr 1fr;gap:6px;margin-bottom:8px"></div>
        <div id="spec-meta" style="font-size:11px"></div>
    `,w);
    fetch('/api').then(r=>r.json()).then(d=>{
        const s=d.sensors||{};
        renderSpectrometer(s);
    });
    setTimeout(()=>{
        fetch('/api').then(r=>r.json()).then(d=>{ renderSpectrometer(d.sensors||{}); });
    },10000);
}

function renderSpectrometer(s){
    const cvs=$('spec-canvas'); if(!cvs) return;
    // AS7343 channels: wavelength order
    const channels = [
        {k:'AS7343_F1_415nm', wl:415, label:'415', col:'#8b5cf6'},
        {k:'AS7343_FZ_450nm', wl:450, label:'450', col:'#6366f1'},
        {k:'AS7343_F2_445nm', wl:445, label:'445', col:'#4f46e5'},
        {k:'AS7343_F3_480nm', wl:480, label:'480', col:'#2563eb'},// 480 blue
        {k:'AS7343_F4_515nm', wl:515, label:'515', col:'#0891b2'},
        {k:'AS7343_FY_555nm', wl:555, label:'555', col:'#16a34a'},
        {k:'AS7343_F5_555nm', wl:555, label:'555b',col:'#22c55e'},
        {k:'AS7343_FXL_600nm',wl:600, label:'600', col:'#ca8a04'},
        {k:'AS7343_F6_640nm', wl:640, label:'640', col:'#d97706'},
        {k:'AS7343_F7_680nm', wl:680, label:'680', col:'#dc2626'},
        {k:'AS7343_F8_910nm', wl:910, label:'910', col:'#be185d'},
    ];

    // Normalize to max
    const vals = channels.map(c=>+(s[c.k]||0));
    const maxVal = Math.max(...vals,1);

    // Draw bars on canvas
    const W=cvs.offsetWidth||480, H=cvs.offsetHeight||120;
    cvs.width=W; cvs.height=H;
    const ctx=cvs.getContext('2d');
    ctx.clearRect(0,0,W,H);

    const barW=Math.floor(W/channels.length)-2;
    channels.forEach((c,i)=>{
        const v=vals[i]; const h=Math.ceil((v/maxVal)*(H-24));
        const x=i*(barW+2)+1;
        // Gradient bar
        const grad=ctx.createLinearGradient(0,H-h,0,H);
        grad.addColorStop(0,c.col+'ff');
        grad.addColorStop(1,c.col+'55');
        ctx.fillStyle=grad;
        ctx.beginPath();
        // Pill shape
        const r=Math.min(barW/2,6);
        ctx.roundRect?ctx.roundRect(x,H-h-16,barW,h,r):ctx.rect(x,H-h-16,barW,h);
        ctx.fill();
        // Label
        ctx.fillStyle='rgba(255,255,255,.5)';
        ctx.font='8px Inter,sans-serif';
        ctx.textAlign='center';
        ctx.fillText(c.label,x+barW/2,H-4);
    });

    // Gain label
    const g=$('spec-gain'); if(g) g.textContent=`GAIN: ${fv(s.AS7343_Gain,1)}X`;

    // UV section (AS7331)
    const uvDiv=$('spec-uv'); if(uvDiv){
        uvDiv.innerHTML=[
            ['UVA','☀️',s.AS7331_UVA,'#fde047'],
            ['UVB','⚡',s.AS7331_UVB,'#fb923c'],
            ['UVC','🔬',s.AS7331_UVC,'#a78bfa'],
        ].map(([lb,ic,val,col])=>`
            <div style="background:rgba(0,0,0,.3);border-radius:8px;padding:10px;text-align:center">
                <div style="font-size:9px;color:var(--text-muted)">${ic} ${lb}</div>
                <div style="font-size:18px;font-weight:700;color:${col}">${fv(val,2)}</div>
                <div style="font-size:9px;color:var(--text-dim)">µW/cm²</div>
            </div>`).join('');
    }

    // Meta info
    const meta=$('spec-meta'); if(meta) meta.innerHTML=`
        <div style="display:grid;grid-template-columns:1fr 1fr;gap:6px;font-size:11px">
            ${[
                ['Medical UVI',fv(s.AS7331_Medical_UVI,2),'','#f59e0b'],
                ['Clear',fvi(s.AS7343_Clear),'ADC','#94a3b8'],
                ['NIR 910nm',fvi(s.AS7343_nIR),'ADC','#be185d'],
                ['TSL2591 Lux',fvi(s.TSL2591_Lux),'lx','#fde68a'],
                ['OPT4048 Lux',fv(s.OPT4048_Lux,1),'lx','#fde68a'],
                ['Melanopic',fv(s.OPTICS_Melanopic_Lux,1),'mlux','#c084fc'],
            ].map(([lb,v,un,c])=>`
                <div style="background:rgba(255,255,255,.04);border-radius:6px;padding:6px 8px">
                    <div style="font-size:9px;color:var(--text-muted)">${lb}</div>
                    <div style="font-weight:600;color:${c}">${v} <span style="font-size:9px;color:var(--text-dim)">${un}</span></div>
                </div>`).join('')}
        </div>
        ${s.OPTICS_Melatonin_Status?`<div style="margin-top:8px;font-size:11px;color:var(--text-dim)">
            🌙 Melatonin: <strong style="color:var(--accent)">${s.OPTICS_Melatonin_Status}</strong></div>`:''}`;
}

// ═══════════════════════════════════════════════════════════════════
// 🌡️ THERMAL CAMERA WINDOW
// ═══════════════════════════════════════════════════════════════════
let thermalTimer=null;
function loadThermal(){
    const vw=window.innerWidth, w=Math.min(vw-16,440);
    createWindow('therm-win','🌡️ Thermal Camera (MLX90640)','🌡️',40,40,`
        <div style="font-size:10px;color:var(--text-muted);text-align:center;margin-bottom:6px">
            32×24 px &nbsp;|&nbsp; ~55°×35° FOV &nbsp;|&nbsp; Sky-facing IR
        </div>
        <div style="position:relative;border-radius:10px;overflow:hidden;background:#000">
            <canvas id="therm-canvas" style="width:100%;height:auto;display:block;image-rendering:pixelated"></canvas>
            <div id="therm-overlay" style="position:absolute;inset:0;display:flex;align-items:center;justify-content:center;
                                           color:rgba(255,255,255,.4);font-size:12px">Loading…</div>
        </div>
        <div style="display:flex;gap:6px;margin-top:8px;font-size:11px" id="therm-stats"></div>
        <div style="margin-top:6px" id="therm-sky"></div>
        <div style="display:flex;gap:6px;margin-top:8px">
            <button class="btn" onclick="startThermal()" style="flex:1">▶ Live</button>
            <button class="btn" onclick="stopThermal()">⏹ Stop</button>
            <button class="btn" onclick="captureThermal()">📷 Snap</button>
        </div>
    `,w);
    startThermal();
}

function startThermal(){
    stopThermal();
    fetchThermalFrame();
}
function stopThermal(){ if(thermalTimer){clearTimeout(thermalTimer);thermalTimer=null;} }

function fetchThermalFrame(){
    fetch('/thermal_json')
        .then(async r=>{
            const data = await r.json();
            if(!r.ok) throw new Error(data && data.error ? data.error : ('HTTP '+r.status));
            return data;
        })
        .then(data=>{
            if(!data || !Array.isArray(data.data) || data.data.length===0){
                throw new Error('Empty thermal frame');
            }
            renderThermal(data);
            thermalTimer=setTimeout(fetchThermalFrame,3000);
        }).catch(err=>{
            const ov=$('therm-overlay');
            if(ov){
                ov.style.display='flex';
                ov.textContent='Offline: '+(err && err.message ? err.message : err);
            }
            thermalTimer=setTimeout(fetchThermalFrame,5000);
        });
}

// Iron-bow palette (black→blue→magenta→red→yellow→white)
const IRON_BOW=[
    [0,0,0],[2,6,70],[3,16,127],[5,30,180],[10,53,200],[40,90,210],
    [80,130,220],[100,160,200],[120,180,180],[140,200,120],[180,210,60],
    [220,200,20],[240,160,10],[255,120,5],[255,80,0],[255,200,50],[255,255,200],[255,255,255]
];
function ironBow(t){
    const i=Math.min(IRON_BOW.length-2,Math.floor(t*(IRON_BOW.length-1)));
    const f=t*(IRON_BOW.length-1)-i;
    const a=IRON_BOW[i],b=IRON_BOW[i+1];
    return [a[0]+(b[0]-a[0])*f, a[1]+(b[1]-a[1])*f, a[2]+(b[2]-a[2])*f];
}

function renderThermal(data){
    const cvs=$('therm-canvas'); if(!cvs) return;
    const W=data.w||32, H=data.h||24, frame=data.data;
    const mn=data.min, mx=data.max, range=mx-mn||1;

    cvs.width=W; cvs.height=H;
    cvs.style.maxHeight='360px';
    const ctx=cvs.getContext('2d');
    const img=ctx.createImageData(W,H);

    for(let i=0;i<W*H;i++){
        const t=(frame[i]-mn)/range;
        const [r,g,b]=ironBow(t);
        img.data[i*4]=r; img.data[i*4+1]=g; img.data[i*4+2]=b; img.data[i*4+3]=255;
    }
    ctx.putImageData(img,0,0);

    const ov=$('therm-overlay'); if(ov) ov.style.display='none';

    // Stats
    const st=$('therm-stats'); if(st) st.innerHTML=`
        <div style="flex:1;background:rgba(0,0,0,.3);border-radius:6px;padding:6px 8px;text-align:center">
            <div style="font-size:9px;color:var(--text-muted)">MIN</div>
            <div style="color:#4fc3f7;font-weight:700">${mn.toFixed(1)}°C</div>
        </div>
        <div style="flex:1;background:rgba(0,0,0,.3);border-radius:6px;padding:6px 8px;text-align:center">
            <div style="font-size:9px;color:var(--text-muted)">MAX</div>
            <div style="color:#ef5350;font-weight:700">${mx.toFixed(1)}°C</div>
        </div>
        <div style="flex:1;background:rgba(0,0,0,.3);border-radius:6px;padding:6px 8px;text-align:center">
            <div style="font-size:9px;color:var(--text-muted)">RANGE</div>
            <div style="color:var(--accent);font-weight:700">${range.toFixed(1)}°C</div>
        </div>`;

    // Sky analysis (from api)
    fetch('/api').then(r=>r.json()).then(d=>{
        const s=d.sensors||{};
        const sky=$('therm-sky'); if(!sky) return;
        const cov=+s.MLX_Cloud_Cover_Pct||0;
        const covCol=cov>75?'#60a5fa':cov>25?'#fbbf24':'#4ade80';
        sky.innerHTML=`<div style="display:flex;align-items:center;gap:10px">
            <div style="flex:1;background:rgba(255,255,255,.07);border-radius:4px;height:6px;overflow:hidden">
                <div style="width:${cov}%;height:100%;background:${covCol};border-radius:4px"></div>
            </div>
            <div style="font-size:11px;color:${covCol};white-space:nowrap">
                ${s.MLX_Sky_Condition||'--'} (${cov.toFixed(0)}%)
            </div>
        </div>
        <div style="font-size:10px;color:var(--text-muted);margin-top:4px">
            Sky T: ${fv(s.MLX_Sky_Temp_C,1)}°C
        </div>`;
    }).catch(()=>{});
}

function captureThermal(){
    const cvs=$('therm-canvas'); if(!cvs) return;
    const a=document.createElement('a');
    a.href=cvs.toDataURL('image/png');
    a.download='thermal_'+Date.now()+'.png';
    a.click();
    showToast('Thermal snapshot saved');
}

// ── Pre-populate history so charts work on revisit ──
loadSection('dash');
</script>
</body>
</html>
)=====";

void handleRoot() { server.send(200, "text/html", index_html); }

void handleThermalCamera() {
    server.send(501, "text/plain", "Use /thermal_json for Web UI");
}

// /thermal_json — returns 32×24 float array as JSON for Web UI canvas rendering
void handleThermalJSON() {
    if (!mlx90640_ok) {
        server.send(503, "application/json", "{\"error\":\"MLX90640 offline\"}");
        return;
    }
    if (!tcaselect(MUX_MLX90640, CH_MLX90640)) {
        server.send(503, "application/json", "{\"error\":\"MUX select failed\"}");
        return;
    }
    static float frame[32*24];
    if (mlx.getFrame(frame) != 0) {
        server.send(503, "application/json", "{\"error\":\"Frame read failed\"}");
        return;
    }
    // Send as compact JSON: {"min":...,"max":...,"data":[...]}
    float mn = 999, mx = -999;
    for (int i = 0; i < 32*24; i++) {
        if (frame[i] < mn) mn = frame[i];
        if (frame[i] > mx) mx = frame[i];
    }
    String out = "{\"min\":";
    out += String(mn, 1);
    out += ",\"max\":";
    out += String(mx, 1);
    out += ",\"w\":32,\"h\":24,\"data\":[";
    for (int i = 0; i < 32*24; i++) {
        if (i) out += ',';
        out += String(frame[i], 1);
    }
    out += "]}";
    server.send(200, "application/json", out);
}

void handleLogs() {
    server.setContentLength(CONTENT_LENGTH_UNKNOWN);
    server.send(200, "text/plain", "");
    int idx = (logCount < MAX_LOG_LINES) ? 0 : logHead;
    for (int i = 0; i < logCount; i++) {
        server.sendContent(logBuffer[idx]);
        idx = (idx + 1) % MAX_LOG_LINES;
    }
    server.sendContent("");
}

void handleWiFiStatus() {
    String status = (WiFi.status() == WL_CONNECTED) ? "OK" : "OFFLINE";
    server.send(200, "text/plain", status);
}

void handleUpdateComplete() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
    ESP.restart();
}

void handleUpdateUpload() {
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
        esp_task_wdt_reset(); 
        if (!Update.begin(UPDATE_SIZE_UNKNOWN)) { Update.printError(Serial); }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
        if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) { Update.printError(Serial); }
        esp_task_wdt_reset(); 
    } else if (upload.status == UPLOAD_FILE_END) {
        if (Update.end(true)) {
            ATLAS_LOG("\n[WEB OTA] Firmware Flashed Successfully. Commencing Reboot...\n");
        } else {
            Update.printError(Serial);
        }
    }
}

// Generic warning helper for compatibility detection
void warnSensorCompatibility(const char* sensorName, bool condition, const char* warningText) {
    if (condition) {
        ATLAS_LOG("[HW INIT][WARN] %s: %s\n", sensorName, warningText);
    }
}

void scanI2C() {
    ATLAS_LOG("\n==================================================\n");
    ATLAS_LOG("            I2C MULTIPLEXER DIAGNOSTICS           \n");
    ATLAS_LOG("==================================================\n");
    uint8_t muxes[] = {0x70, 0x71, 0x72};
    for(uint8_t m = 0; m < 3; m++) {
        ATLAS_LOG("--> MUX [0x%02X] SCANNING...\n", muxes[m]);
        for (uint8_t t = 0; t < 8; t++) {
            if(!tcaselect(muxes[m], t)) {
                ATLAS_LOG("  |-- CH[%d]: MUX OFFLINE\n", t);
                continue;
            }
            bool foundAny = false;
            ATLAS_LOG("  |-- CH[%d]: ", t);
            for (uint8_t addr = 1; addr < 127; addr++) {
                if (addr == 0x70 || addr == 0x71 || addr == 0x72 || addr == 0x50) continue;
                Wire.beginTransmission(addr);
                if (Wire.endTransmission() == 0) {
                    foundAny = true;
                    ATLAS_LOG("0x%02X ", addr);
                }
            }
            if(!foundAny) ATLAS_LOG("EMPTY");
            ATLAS_LOG("\n");
        }
    }
    ATLAS_LOG("--> SYSTEM COMPONENTS:\n");
    if (i2c_mem_ok) ATLAS_LOG("  |-- I2C EEPROM (0x50): ONLINE (BSEC AI Memory)\n");
    else ATLAS_LOG("  |-- I2C EEPROM (0x50): [ERROR] OFFLINE / MISSING\n");
    ATLAS_LOG("==================================================\n\n");
}

void handleCmd() {
    String act = server.arg("action");
    String mux_str = server.arg("mux");
    String ch_str = server.arg("ch");
    String sensor = server.arg("sensor");

    server.send(200, "text/plain", "Command Received: " + act);

    uint8_t mux = (mux_str.length() > 0) ? (uint8_t)strtol(mux_str.c_str(), NULL, 0) : 0;
    uint8_t ch = (ch_str.length() > 0) ? (uint8_t)ch_str.toInt() : 0;

    if (act == "scan") scanI2C();
    else if (act == "read_all") {
        ATLAS_LOG("\n[SYSTEM CMD] Manual Sensor Read Triggered...\n");
        exhaustivelyReadSensors();
    }
    else if (act == "init_sensors") {
        ATLAS_LOG("\n[SYSTEM CMD] Manual Sensor Initialization Sequence...\n");
        initSensors();
    }
    else if (act == "stop_sensors") {
        ATLAS_LOG("\n[SYSTEM CMD] Manual Sensor Shutdown (Deep Sleep Mode)...\n");
        sleepSensors(true);
    }
    else if (act == "sandbox_init") {
        ATLAS_LOG("\n[SANDBOX] Initializing %s on MUX 0x%02X, CH %d\n", sensor.c_str(), mux, ch);
        if (sensor == "BME280") { MUX_BME280 = mux; CH_BME280 = ch; bme280_ok = false; initBME280(); }
        else if (sensor == "BMV080") { MUX_BMV080 = mux; CH_BMV080 = ch; bmv080_ok = false; initBMV080(); }
        else if (sensor == "SCD41") { MUX_SCD41 = mux; CH_SCD41 = ch; scd41_ok = false; initSCD41(); }
        else if (sensor == "BME688") { MUX_BME688 = mux; CH_BME688 = ch; bme688_ok = false; initBME688(); }
        else if (sensor == "SHT45") { MUX_SHT45 = mux; CH_SHT45 = ch; sht45_ok = false; initSHT45(); }
        else if (sensor == "SGP41") { MUX_SGP41 = mux; CH_SGP41 = ch; sgp41_ok = false; initSGP41(); }
        else if (sensor == "BMP585") { MUX_BMP585 = mux; CH_BMP585 = ch; bmp585_ok = false; initBMP585(); }
        else if (sensor == "AS7343") { MUX_AS7343 = mux; CH_AS7343 = ch; as7343_ok = false; initAS7343(); }
        else if (sensor == "OPT4048") { MUX_OPT4048 = mux; CH_OPT4048 = ch; opt4048_ok = false; initOPT4048(); }
        else if (sensor == "TCS34725") { MUX_TCS34725 = mux; CH_TCS34725 = ch; tcs34725_ok = false; initTCS34725(); }
        else if (sensor == "TSL2591") { MUX_TSL2591 = mux; CH_TSL2591 = ch; tsl2591_ok = false; initTSL2591(); }
        else if (sensor == "INA219") { MUX_INA219 = mux; CH_INA219 = ch; ina219_ok = false; initINA219(); }
        else if (sensor == "MS8607") { MUX_MS8607 = mux; CH_MS8607 = ch; ms8607_ok = false; initMS8607(); }
        else if (sensor == "MAX17048") { MUX_MAX17048 = mux; CH_MAX17048 = ch; max17048_ok = false; initMAX17048(); }
        else if (sensor == "VEML7700") { MUX_VEML = mux; CH_VEML = ch; veml_ok = false; initVEML7700(); }
        else if (sensor == "LTR390") { MUX_LTR = mux; CH_LTR = ch; ltr_ok = false; initLTR390(); }
        else if (sensor == "AS3935") { MUX_AS3935 = mux; CH_AS3935 = ch; as3935_ok = false; initAS3935(); }
        else if (sensor == "ILPS22QS") { MUX_ILPS = mux; CH_ILPS = ch; ilps_ok = false; initILPS22QS(); }
        else if (sensor == "AS7331") { MUX_AS7331 = mux; CH_AS7331 = ch; as7331_ok = false; initAS7331(); }
        else if (sensor == "I2CMEM") { MUX_I2CMEM = mux; CH_I2CMEM = ch; i2c_mem_ok = false; initI2CMemory(); }
    }
    else if (act == "reboot") { ATLAS_LOG("\n[SYSTEM CMD] Reboot requested...\n"); delay(1000); ESP.restart(); }
    else if (act == "reset_i2c") { 
        ATLAS_LOG("\n[SYSTEM CMD] Hard resetting I2C Bus & Clearing Fault Flags...\n");
        Wire.end(); delay(200); Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
        Wire.setClock(100000); 
        bme688_ok = bmv080_ok = max17048_ok = ms8607_ok = ina219_ok = as3935_ok = bme280_ok = as7343_ok = veml_ok = ltr_ok = false;
        sht45_ok = sgp41_ok = tsl2591_ok = opt4048_ok = tcs34725_ok = scd41_ok = bmp585_ok = ilps_ok = as7331_ok = mlx90640_ok = false;
        initSensors(); 
    }
    else if (act == "mode_cont") { 
        currentMode = MODE_CONTINUOUS;
        prefs.begin("sys", false); prefs.putInt("mode", 0); prefs.end();
        ATLAS_LOG("\n[SYSTEM CMD] Overriding mode -> CONTINUOUS\n");
    }
    else if (act == "mode_light") { 
        currentMode = MODE_LIGHT_SLEEP;
        prefs.begin("sys", false); prefs.putInt("mode", 2); prefs.end(); 
        ATLAS_LOG("\n[SYSTEM CMD] Overriding mode -> LIGHT SLEEP (Active after next API push)\n");
    }
    else if (act == "mode_deep") { 
        currentMode = MODE_DEEP_SLEEP;
        prefs.begin("sys", false); prefs.putInt("mode", 1); prefs.end(); 
        ATLAS_LOG("\n[SYSTEM CMD] Overriding mode -> DEEP SLEEP (Active after next API push)\n");
    }
    else if (act == "mode_maint") { 
        currentMode = MODE_MAINTENANCE;
        prefs.begin("sys", false); prefs.putInt("mode", 3); prefs.end(); 
        ATLAS_LOG("\n[SYSTEM CMD] Overriding mode -> MAINTENANCE (Active immediately)\n");
    }
    else if (act == "mode_recv") { 
        currentMode = MODE_RECOVERY;
        prefs.begin("sys", false); prefs.putInt("mode", 4); prefs.end(); 
        ATLAS_LOG("\n[SYSTEM CMD] Overriding mode -> RECOVERY (Emergency Pull Down!)\n");
    }
    else if (act == "factory_reset") { 
        ATLAS_LOG("\n[SYSTEM CMD] EXECUTING FACTORY RESET (Wiping AI and Config Caches)...\n");
        prefs.begin("bsec_data", false); prefs.clear(); prefs.end();
        prefs.begin("bmv_cfg", false); prefs.clear(); prefs.end();
        prefs.begin("sys", false); prefs.clear(); prefs.end();
        prefs.begin("diag", false); prefs.clear(); prefs.end();
        prefs.begin("rain", false); prefs.clear(); prefs.end();
        delay(1000); 
        ESP.restart();
    }
}

// ¯\_(ツ)_/¯ ¯\_(ツ)_/¯ ¯\_(ツ)_/¯ ¯\_(ツ)_/¯ ¯\_(ツ)_/¯ ¯\_(ツ)_/¯
// 6. SETUP & WIFI & OTA & NTP & MQTT
// ¯\_(ツ)_/¯ ¯\_(ツ)_/¯ ¯\_(ツ)_/¯ ¯\_(ツ)_/¯ ¯\_(ツ)_/¯ ¯\_(ツ)_/¯

void syncNTP() {
    configTzTime(TZ_INFO, NTP_SERVER_1, NTP_SERVER_2);
    time_t now = time(nullptr);
    unsigned long start = millis();
    ATLAS_LOG("[NETWORK] Syncing NTP Time...");
    while (now < 1600000000 && (millis() - start < NTP_TIMEOUT_MS)) { delay(100); now = time(nullptr); }
    if (now > 1600000000) ATLAS_LOG(" SUCCESS\n");
    else ATLAS_LOG(" FAILED\n");
}

bool setupWiFiAndOTA() {
    if(WiFi.status() == WL_CONNECTED) return true;
    ATLAS_LOG("\n[NETWORK] Commencing WiFi Scan & Connect Sequence...\n");
    
    WiFi.mode(WIFI_STA);
    WiFi.disconnect(true);
    delay(100); 
    
    int n = WiFi.scanNetworks();
    String targetSSID = "";
    
    if (n == 0) {
        ATLAS_LOG("[NETWORK] Scan complete. No networks found in range.\n");
        esp_wifi_set_ps(WIFI_PS_NONE); 
        return false;
    }

    for (size_t j = 0; j < WIFI_SSID_COUNT; j++) {
        for (int i = 0; i < n; ++i) {
            if (WiFi.SSID(i) == WIFI_SSIDS[j]) {
                targetSSID = WIFI_SSIDS[j];
                ATLAS_LOG("  -> Discovered Priority SSID: %s (RSSI: %d dBm)\n", targetSSID.c_str(), WiFi.RSSI(i));
                break;
            }
        }
        if (targetSSID != "") break;
    }
    
    if (targetSSID == "") {
        ATLAS_LOG("[NETWORK] CRITICAL: None of the whitelisted SSIDs are available.\n");
        esp_wifi_set_ps(WIFI_PS_NONE); 
        return false;
    }

    ATLAS_LOG("[NETWORK] Priority Target Locked: %s. Initiating handshake...\n", targetSSID.c_str());
    for (int attempt = 0; attempt < 2; attempt++) {
        String pwrMode = (attempt == 0) ? "MIN_MODEM" : "NONE (Full Power)";
        if(attempt == 0) esp_wifi_set_ps(WIFI_PS_MIN_MODEM);
        else esp_wifi_set_ps(WIFI_PS_NONE);

        WiFi.begin(targetSSID.c_str(), WIFI_PASSWORD);
        unsigned long startTimer = millis();
        ATLAS_LOG("[NETWORK] Connecting [%s] ", pwrMode.c_str());
        
        while (WiFi.status() != WL_CONNECTED && (millis() - startTimer < 8000)) { 
            delay(500);
            ATLAS_LOG("."); 
        }
        
        if (WiFi.status() == WL_CONNECTED) {
            ATLAS_LOG("\n[NETWORK] Uplink Established! IP: %s\n", WiFi.localIP().toString().c_str());
            if(MDNS.begin(HOSTNAME)) ATLAS_LOG("[NETWORK] mDNS Address: http://%s.local\n", HOSTNAME);
            ArduinoOTA.setHostname(HOSTNAME); ArduinoOTA.begin();
            syncNTP();
            return true;
        } else {
            ATLAS_LOG(" TIMEOUT.\n");
            WiFi.disconnect(true);
        }
    }
    
    ATLAS_LOG("[NETWORK] Handshake failed despite presence.\n");
    return false;
}

// [TODO_SW#0003]
// ZAIMPLEMENTOWAC DWIE RZECZY:
// 
// "recovery" gdzie przechodzi
// i wczytuje tylko panel recovery --- anulowac dzialanie calego programu natychmiast
// uruchomic go w trybie awaryjnymn z WIFI, MDNS, OTA UPDATE, HTTP GUI FW UPGRADE
// mozna to zrobic wymuszajac w locie zapis do NVS flagi recovery, aby 
// przy ponownym starcie miec pewnosc ze nie pojdzie dalej - do przeanalizowania
// niech zweryfikuje scenariusze gemini
//
// "recovery:https://link.url/img.bin" - gdzie podany jest link do fw ktory ma sobie
// pociagnac i zupgrade'owac
// w tym przypadku rozwazyc implementacje karty SD i recovery.bin na niej jumiescic
void checkRemoteModeOverride() {
    if (WiFi.status() != WL_CONNECTED) return;
    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;
    http.begin(client, MODE_OVERRIDE_URL);
    http.setTimeout(3000);
    int httpCode = http.GET();
    if (httpCode == 200) {
        String remotePayload = http.getString();
        remotePayload.trim();
        remotePayload.toLowerCase();
        if (remotePayload == "continuous") {
            currentMode = MODE_CONTINUOUS;
            ATLAS_LOG("[SYSTEM] Remote Override: Mode set to CONTINUOUS\n");
        } else if (remotePayload == "lightsleep" || remotePayload == "light") {
            currentMode = MODE_LIGHT_SLEEP;
            ATLAS_LOG("[SYSTEM] Remote Override: Mode set to LIGHT SLEEP\n");
        } else if (remotePayload == "deepsleep" || remotePayload == "deep") {
            currentMode = MODE_DEEP_SLEEP;
            ATLAS_LOG("[SYSTEM] Remote Override: Mode set to DEEP SLEEP\n");
        } else if (remotePayload == "maintenance" || remotePayload == "admin") {
            currentMode = MODE_MAINTENANCE;
            ATLAS_LOG("[SYSTEM] Remote Override: Mode set to MAINTENANCE\n");
        } else if (remotePayload == "recovery" || remotePayload == "emergency") {
            currentMode = MODE_RECOVERY;
            ATLAS_LOG("[SYSTEM] Remote Override: Mode set to RECOVERY (Emergency Pull Down)\n");
        }
        prefs.begin("sys", false);
        int mode_val = 0;
        if (currentMode == MODE_DEEP_SLEEP) mode_val = 1;
        else if (currentMode == MODE_LIGHT_SLEEP) mode_val = 2;
        else if (currentMode == MODE_MAINTENANCE) mode_val = 3;
        else if (currentMode == MODE_RECOVERY) mode_val = 4;
        prefs.putInt("mode", mode_val); 
        prefs.end();
    }
    http.end();
}

void setup() {
    Serial.begin(115200);
    
    esp_reset_reason_t reason = esp_reset_reason();
    if (reason == ESP_RST_POWERON || reason == ESP_RST_BROWNOUT) {
        Serial.println("\n[SYSTEM] True Cold Boot Detected. Enforcing Golden Configurations...");
        prefs.begin("bsec_data", false); prefs.clear(); prefs.end();
        Serial.println("[SYSTEM] BME688: Cleared AI baseline. Forcing new Run-In and calibration cycle.");
        
        prefs.begin("bmv_cfg", false);
        prefs.putUShort("intTime", 10);
        prefs.putBool("vibFilter", true); 
        prefs.putBool("obsDetect", false); 
        prefs.putUChar("algo", 2);        
        prefs.end();
        Serial.println("[SYSTEM] BMV080: Enforced High-Precision parameters for outdoor operations.");
    }
    
    pinMode(I2C_SDA_PIN, INPUT_PULLUP);
    pinMode(I2C_SCL_PIN, INPUT_PULLUP);
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
    Wire.setClock(100000); 
    Wire.setTimeOut(100);
    
    // Konfiguracja Watchdoga dla ESP32 Arduino Core v2.0.x (platforma espressif32@6.5.0)
    esp_task_wdt_init(WDT_TIMEOUT_SECONDS, true);
    esp_task_wdt_add(NULL);

    mqtt.setBufferSize(4096); 
    
    prefs.begin("sys", true);
    int m = prefs.getInt("mode", 0);
    currentMode = (m == 1) ? MODE_DEEP_SLEEP : ((m == 2) ? MODE_LIGHT_SLEEP : ((m == 3) ? MODE_MAINTENANCE : ((m == 4) ? MODE_RECOVERY : MODE_CONTINUOUS)));
    prefs.end();

    server.on("/", handleRoot);
    server.on("/logs", handleLogs);
    server.on("/wifi", handleWiFiStatus);
    server.on("/api", HTTP_GET, []() {
        String out;
        serializeJson(payload, out);
        server.send(200, "application/json", out);
    });
    // /scan_json — returns structured I2C bus scan as JSON (used by Web UI)
    server.on("/scan_json", HTTP_GET, []() {
        JsonDocument doc;
        uint8_t muxes[] = {0x70, 0x71, 0x72};
        const char* mux_names[] = {"0x70","0x71","0x72"};
        JsonArray mux_arr = doc["muxes"].to<JsonArray>();
        for (uint8_t m = 0; m < 3; m++) {
            JsonObject mux_obj = mux_arr.add<JsonObject>();
            mux_obj["addr"] = mux_names[m];
            JsonArray chs = mux_obj["channels"].to<JsonArray>();
            for (uint8_t t = 0; t < 8; t++) {
                JsonObject ch_obj = chs.add<JsonObject>();
                ch_obj["ch"] = t;
                JsonArray dev_arr = ch_obj["devices"].to<JsonArray>();
                if (!tcaselect(muxes[m], t)) {
                    ch_obj["status"] = "OFFLINE";
                } else {
                    ch_obj["status"] = "OK";
                    for (uint8_t addr = 1; addr < 127; addr++) {
                        if (addr == 0x70 || addr == 0x71 || addr == 0x72) continue;
                        Wire.beginTransmission(addr);
                        if (Wire.endTransmission() == 0) {
                            char hex[7]; snprintf(hex, sizeof(hex), "0x%02X", addr);
                            dev_arr.add(hex);
                        }
                    }
                }
            }
        }
        // Also scan main bus (no MUX)
        tcaselect(0x70, 0xFF); // disable all channels
        JsonObject main_obj = doc["main_bus"].to<JsonObject>();
        JsonArray main_arr = main_obj["devices"].to<JsonArray>();
        for (uint8_t addr = 1; addr < 127; addr++) {
            Wire.beginTransmission(addr);
            if (Wire.endTransmission() == 0) {
                char hex[7]; snprintf(hex, sizeof(hex), "0x%02X", addr);
                main_arr.add(hex);
            }
        }
        String out; serializeJson(doc, out);
        server.send(200, "application/json", out);
    });
    server.on("/cmd", handleCmd);
    server.on("/thermal.bmp", handleThermalCamera);
    server.on("/thermal_json", HTTP_GET, handleThermalJSON);
    server.on("/update", HTTP_POST, handleUpdateComplete, handleUpdateUpload);
    
    if(esp_sleep_get_wakeup_cause() != ESP_SLEEP_WAKEUP_TIMER && esp_sleep_get_wakeup_cause() != ESP_SLEEP_WAKEUP_GPIO) {
        dynamic_warmup_ms = 45000; 
        setupWiFiAndOTA();
        checkRemoteModeOverride();
        server.begin();
        initSensors();
        scanI2C();
        ATLAS_LOG("\n[SYSTEM] Power Cycle -> Entering Warmup Phase to allow sensors to stabilize.\n");
        currentPhase = PHASE_WARMUP;
    } else {
        dynamic_warmup_ms = 45000;
        WiFi.mode(WIFI_OFF);
        initSensors();
        ATLAS_LOG("\n[SYSTEM] Deep Sleep Wakeup -> Initializing Silent Warmup Phase.\n");
        currentPhase = PHASE_WARMUP;
    }
    cycleStartTime = millis();
}

// ~~~~~^~~~~~^~~~~~^~~~~~^~~~~~^~~~~~^~~~~~^~~~~~^~~~~~^~~~~~^~~~~~^~~~~~
// 7. PUSH API, WEATHER UNDERGROUND & MQTT
// ~~~~~^~~~~~^~~~~~^~~~~~^~~~~~^~~~~~^~~~~~^~~~~~^~~~~~^~~~~~^~~~~~^~~~~~

void pushMQTT() {
    if (WiFi.status() != WL_CONNECTED) return;
    if (!mqtt.connected()) {
        mqtt.setServer(MQTT_SERVER, MQTT_PORT);
        ATLAS_LOG("[MQTT] Connecting to Home Assistant Broker at %s...\n", MQTT_SERVER);
        if (mqtt.connect(HOSTNAME, MQTT_USER, MQTT_PASS)) {
            ATLAS_LOG("[MQTT] Uplink Established! Generating Auto-Discovery parameters...\n");
        } else {
            ATLAS_LOG("[MQTT] Connection Failed, rc=%d\n", mqtt.state());
            return;
        }
    }

    JsonObject sensors = payload["sensors"].as<JsonObject>();
    for (JsonPair kv : sensors) {
        String key = kv.key().c_str();
        bool found = false;
        for (String k : discovered_sensors) { if (k == key) { found = true; break; } }
        
        if (!found) {
            String domain = "sensor";
            if (key.indexOf("Fault_") >= 0) {
                domain = "binary_sensor";
            }
            
            String topic = String("homeassistant/") + domain + "/" + HOSTNAME + "/" + key + "/config";
            JsonDocument doc;
            
            // ===============================================================
            // DICTIONARY: ENGLISH NAMES & ICONS
            // ===============================================================
            String friendlyName = key;
            String icon = "";

            if (key == "BME280_Enc_Temp") { friendlyName = "Enclosure Temperature"; icon = "mdi:thermometer"; }
            else if (key == "BME280_Enc_Hum") { friendlyName = "Enclosure Humidity"; icon = "mdi:water-percent"; }
            else if (key == "BME280_Enc_Press") { friendlyName = "Enclosure Pressure"; icon = "mdi:gauge"; }
            else if (key == "BMV080_PM1_0") { friendlyName = "PM1.0 Particulates"; icon = "mdi:blur"; }
            else if (key == "BMV080_PM2_5") { friendlyName = "PM2.5 Particulates"; icon = "mdi:blur"; }
            else if (key == "BMV080_PM10_0") { friendlyName = "PM10 Particulates"; icon = "mdi:blur"; }
            else if (key == "BMV080_Num_PM1_0") { friendlyName = "PM1.0 Particles Count"; icon = "mdi:scatter-plot"; }
            else if (key == "BMV080_Num_PM2_5") { friendlyName = "PM2.5 Particles Count"; icon = "mdi:scatter-plot"; }
            else if (key == "BMV080_Num_PM10_0") { friendlyName = "PM10 Particles Count"; icon = "mdi:scatter-plot"; }
            else if (key == "BMV080_Obstructed") { friendlyName = "Laser Obstruction Status"; icon = "mdi:alert-circle"; }
            else if (key == "BMV080_Out_Of_Range") { friendlyName = "PM Measurement Out of Range"; icon = "mdi:alert-octagram"; }
            else if (key == "SOLAR_Bus_Voltage") { friendlyName = "Solar Bus Voltage"; icon = "mdi:solar-power"; }
            else if (key == "SOLAR_Load_Voltage") { friendlyName = "Solar Load Voltage"; icon = "mdi:solar-power"; }
            else if (key == "SOLAR_Shunt_mV") { friendlyName = "Solar Shunt Voltage"; icon = "mdi:resistor"; }
            else if (key == "SOLAR_Current_mA") { friendlyName = "Solar Current Draw"; icon = "mdi:current-dc"; }
            else if (key == "SOLAR_Power_mW") { friendlyName = "Solar Power Yield"; icon = "mdi:solar-panel-large"; }
            else if (key == "SOLAR_Efficiency_Pct") { friendlyName = "Solar Panel Efficiency"; icon = "mdi:percent-circle"; }
            else if (key == "ENV_Ext_Temp") { friendlyName = "External Temperature"; icon = "mdi:thermometer-exterior"; }
            else if (key == "ENV_Ext_Hum") { friendlyName = "External Humidity"; icon = "mdi:water-percent"; }
            else if (key == "ENV_Ext_Press") { friendlyName = "External Pressure"; icon = "mdi:gauge"; }
            else if (key == "METEO_Dew_Point_C") { friendlyName = "Dew Point"; icon = "mdi:thermometer-water"; }
            else if (key == "METEO_Cloud_Base_m") { friendlyName = "Estimated Cloud Base"; icon = "mdi:cloud-arrow-down"; }
            else if (key == "METEO_Abs_Hum_g_m3") { friendlyName = "Absolute Humidity"; icon = "mdi:water"; }
            else if (key == "METEO_Sea_Level_Press_hPa") { friendlyName = "Sea Level Pressure"; icon = "mdi:gauge"; }
            else if (key == "METEO_Heat_Index")    { friendlyName = "Heat Index";                      icon = "mdi:sun-thermometer"; }
            else if (key == "METEO_Feels_Like_C") { friendlyName = "Feels Like (NOAA)"; icon = "mdi:thermometer-lines"; doc["dev_cla"] = "temperature"; doc["unit_of_meas"] = "°C"; }
            else if (key == "METEO_Air_Density") { friendlyName = "Air Density"; icon = "mdi:weather-windy"; }
            else if (key == "METEO_Mold_Risk") { friendlyName = "Mold Growth Risk"; icon = "mdi:fungus"; }
            else if (key == "METEO_Virus_Risk") { friendlyName = "Airborne Virus Risk"; icon = "mdi:virus"; }
            else if (key == "METEO_Wet_Bulb_C") { friendlyName = "Wet Bulb Temperature"; icon = "mdi:thermometer-lines"; }
            else if (key == "BME688_Temp_Raw") { friendlyName = "Raw MOX Temperature"; icon = "mdi:thermometer-minus"; }
            else if (key == "BME688_Hum_Raw") { friendlyName = "Raw MOX Humidity"; icon = "mdi:water-percent"; }
            else if (key == "BME688_Press_Raw") { friendlyName = "Raw MOX Pressure"; icon = "mdi:gauge"; }
            else if (key == "BME688_Comp_Temp") { friendlyName = "AI Compensated Temp"; icon = "mdi:thermometer-auto"; }
            else if (key == "BME688_Comp_Hum") { friendlyName = "AI Compensated Hum"; icon = "mdi:water-percent"; }
            else if (key == "BME688_IAQ") { friendlyName = "Outdoor Gas Index (bOAQ)"; icon = "mdi:pine-tree-fire"; }
            else if (key == "BME688_eCO2") { friendlyName = "Ambient Organic Carbon (eCO2)"; icon = "mdi:molecule-co2"; }
            else if (key == "BME688_bVOC") { friendlyName = "Ambient VOCs Estimate"; icon = "mdi:gas-cylinder"; }
            else if (key == "BME688_Gas_Res_Raw") { friendlyName = "MOX Heater Resistance"; icon = "mdi:resistor"; }
            else if (key == "BME688_Static_IAQ") { friendlyName = "Static Gas Index"; icon = "mdi:smog"; }
            else if (key == "BME688_Gas_Percentage") { friendlyName = "Gas Percentage Estimate"; icon = "mdi:percent"; }
            else if (key == "BME688_Comp_Gas") { friendlyName = "Compensated Gas Resistance"; icon = "mdi:resistor"; }
            else if (key == "BME688_IAQ_Accuracy") { friendlyName = "BSEC AI Calibration Level"; icon = "mdi:brain"; }
            else if (key == "BME688_IAQ_Status") { friendlyName = "Outdoor Gas Quality Status"; icon = "mdi:air-filter"; }
            else if (key == "BME688_Stab_Status") { friendlyName = "BSEC Stabilization Status"; icon = "mdi:progress-clock"; }
            else if (key == "BME688_RunIn_Status") { friendlyName = "BSEC Run-In Status"; icon = "mdi:progress-wrench"; }
            else if (key == "BME688_Gas_Est_1") { friendlyName = "AI Gas Estimate 1"; icon = "mdi:flask-outline"; }
            else if (key == "BME688_Gas_Est_2") { friendlyName = "AI Gas Estimate 2"; icon = "mdi:flask-outline"; }
            else if (key == "BME688_Gas_Est_3") { friendlyName = "AI Gas Estimate 3"; icon = "mdi:flask-outline"; }
            else if (key == "BME688_Gas_Est_4") { friendlyName = "AI Gas Estimate 4"; icon = "mdi:flask-outline"; }
            else if (key == "GAS_Identified_Pattern") { friendlyName = "Gas Pattern Identification"; icon = "mdi:brain"; }
            else if (key == "GAS_Type_Category") { friendlyName = "Gas Type Category"; icon = "mdi:flask-round"; }
            else if (key == "GAS_Toxicity_Risk") { friendlyName = "Gas Toxicity Risk (0-10)"; icon = "mdi:alert-octagon"; }
            else if (key == "GAS_Toxicity_Name") { friendlyName = "Toxicity Level Name"; icon = "mdi:alert"; }
            else if (key == "GAS_IAQ_Score") { friendlyName = "Indoor Air Quality Score"; icon = "mdi:air-filter"; }
            else if (key == "BME688_Raw_Gas_Index") { friendlyName = "Raw Gas Index (Normalized)"; icon = "mdi:chart-bell-curve"; }
            else if (key == "BMS_Cell_Voltage") { friendlyName = "Battery Cell Voltage"; icon = "mdi:battery-outline"; }
            else if (key == "BMS_State_Of_Charge") { friendlyName = "Battery State of Charge"; icon = "mdi:battery-50"; }
            else if (key == "BMS_Charge_Rate") { friendlyName = "Battery Drain Rate"; icon = "mdi:battery-clock"; }
            else if (key == "BMS_Remaining_Capacity_mAh") { friendlyName = "Remaining Capacity"; icon = "mdi:battery-charging-100"; }
            else if (key == "BMS_ESP_Power_mW") { friendlyName = "Calculated ESP32 Power Draw"; icon = "mdi:flash"; }
            else if (key == "BMS_ESP_Current_mA") { friendlyName = "Calculated ESP32 Current Load"; icon = "mdi:current-dc"; }
            else if (key == "BMS_Time_To_Empty_min") { friendlyName = "Est. Time to Empty"; icon = "mdi:timer-sand"; }
            else if (key == "BMS_Time_To_Full_Current_min") { friendlyName = "Est. Time to Full (Current)"; icon = "mdi:timer-sand-full"; }
            else if (key == "BMS_Time_To_Full_100_min") { friendlyName = "Est. Time to Full (Ideal)"; icon = "mdi:timer-sand-full"; }
			else if (key == "SCD41_CO2_ppm") { friendlyName = "Absolute CO2 (NDIR)"; icon = "mdi:molecule-co2"; }
            else if (key == "SCD41_Temp") { friendlyName = "SCD41 Temperature (Internal)"; icon = "mdi:thermometer-minus"; }
            else if (key == "SCD41_Hum") { friendlyName = "SCD41 Humidity (Internal)"; icon = "mdi:water-percent"; }
            else if (key == "BMP585_Pressure_hPa") { friendlyName = "QNH Pressure (BMP585)"; icon = "mdi:gauge"; }
            else if (key == "BMP585_Temp") { friendlyName = "BMP585 Sensor Temp"; icon = "mdi:thermometer"; }
            else if (key == "BMP585_dP_dt") { friendlyName = "Pressure Delta (dP/dt)"; icon = "mdi:trending-up"; }
            else if (key == "ILPS22QS_Press_hPa") { friendlyName = "Electrostatic Pressure (ILPS)";
            icon = "mdi:gauge"; }
            else if (key == "ILPS22QS_Temp_C") { friendlyName = "Electrostatic Sensor Temp";
            icon = "mdi:thermometer"; }
            else if (key == "ILPS22QS_QVAR_mV") { friendlyName = "QVAR Electrostatic Charge";
            icon = "mdi:lightning-bolt-outline"; }
            else if (key == "RG15_Acc_mm") { friendlyName = "Rain Accumulation"; icon = "mdi:weather-pouring"; }
            else if (key == "RG15_Daily_Rain_mm") { friendlyName = "Daily Rain Accumulation"; icon = "mdi:weather-pouring"; }
            else if (key == "RG15_EventAcc_mm") { friendlyName = "Rain Event Accumulation"; icon = "mdi:weather-rainy"; }
            else if (key == "RG15_TotalAcc_mm") { friendlyName = "Rain Total Accumulation"; icon = "mdi:water-plus"; }
            else if (key == "RG15_Intensity_mmph") { friendlyName = "Rain Intensity"; icon = "mdi:weather-lightning-rainy"; }
            else if (key == "AS7331_Medical_UVI") { friendlyName = "Medical UV Index"; icon = "mdi:white-balance-sunny"; }
            else if (key == "AS7343_Sodium_Pollution") { friendlyName = "Sodium Light Pollution"; icon = "mdi:lamp"; }
            else if (key == "AS7343_Bortle_Class") { friendlyName = "Bortle Sky Class"; icon = "mdi:weather-night"; }
            else if (key == "METEO_Density_Altitude_m") { friendlyName = "Density Altitude"; icon = "mdi:airplane-takeoff"; }
            else if (key == "METEO_VPD_kPa") { friendlyName = "Vapor Pressure Deficit"; icon = "mdi:water-thermometer"; }
            else if (key == "METEO_Boiling_Point_C") { friendlyName = "Water Boiling Point"; icon = "mdi:pot-steam"; }
            else if (key == "METEO_Pressure_3h_Delta") { friendlyName = "Trend Ciśnienia (3h)"; icon = "mdi:chart-line-variant"; }
            else if (key == "AVIATION_TAS_Proxy_kt") { friendlyName = "TAS Proxy (100kt base)"; icon = "mdi:airplane"; }
            else if (key == "AVIATION_Icing_Risk") { friendlyName = "Icing Risk Index"; icon = "mdi:snowflake-alert"; }
            else if (key == "AVIATION_QFE_hPa") { friendlyName = "QFE Pressure"; icon = "mdi:gauge"; }
            else if (key == "Geiger_CPM_Corrected") { friendlyName = "GCR Corrected (Space Weather)"; icon = "mdi:radioactive-circle"; }
            else if (key == "MED_Respiratory_Hazard_Index") { friendlyName = "Respiratory Hazard Index"; icon = "mdi:lungs-arrow-right"; }
            // ── BIOMETEOROLOGICAL PAIN MODELS v2.0 ──
            else if (key == "MED_Migraine_Risk")    { friendlyName = "Migraine Risk (0–10)";     icon = "mdi:brain";                     doc["unit_of_meas"] = "/10"; }
            else if (key == "MED_Migraine_Cat")     { friendlyName = "Migraine Category";        icon = "mdi:brain"; }
            else if (key == "MED_Rheumatic_Risk")   { friendlyName = "Rheumatic Pain Risk (0–10)";icon = "mdi:bone";                    doc["unit_of_meas"] = "/10"; }
            else if (key == "MED_Rheumatic_Cat")    { friendlyName = "Rheumatic Category";       icon = "mdi:bone"; }
            else if (key == "MED_Baro_Pain_Index")  { friendlyName = "Barometric Pain Index";    icon = "mdi:gauge-low";                 doc["unit_of_meas"] = "/10"; }
            else if (key == "MED_Sinus_Risk")       { friendlyName = "Sinus Congestion Risk";    icon = "mdi:emoticon-sick-outline";     doc["unit_of_meas"] = "/10"; }
            else if (key == "MED_Biometeo_Score")   { friendlyName = "Biometeo Sensitivity Score";icon = "mdi:weather-cloudy-clock";    doc["unit_of_meas"] = "/10"; }
            else if (key == "MED_Biometeo_Alert")   { friendlyName = "Biometeo Alert Level";     icon = "mdi:alert-circle"; }
            else if (key == "METEO_Temp_3h_Delta")  { friendlyName = "Temperature Change (3h)";  icon = "mdi:thermometer-chevron-up";   doc["unit_of_meas"] = "°C"; }
            // ── legacy removed (replaced above) ──
            else if (key == "MED_Joint_Pain_Risk")  { friendlyName = "Joint Pain Risk (legacy)"; icon = "mdi:bone"; }
            else if (key == "MED_Stroke_Risk") { friendlyName = "Stroke Risk (Biometeo)"; icon = "mdi:heart-pulse"; }
            else if (key == "MED_Asthma_Risk") { friendlyName = "Asthma Risk (Environment)"; icon = "mdi:lungs"; }
            else if (key == "ENV_Fire_Risk_Pct") { friendlyName = "Wildfire Risk Index"; icon = "mdi:fire-alert"; }
            else if (key == "OPTICS_Melatonin_Status") { friendlyName = "Wydzielanie Melatonininy"; icon = "mdi:moon-waning-crescent"; }
            else if (key == "SOLAR_ESP_Utilization_pct") { friendlyName = "Solar Utilization by ESP32"; icon = "mdi:solar-power"; }
            else if (key == "SOLAR_ESP_vs_max_pct") { friendlyName = "ESP32 Load vs Max Solar"; icon = "mdi:chart-pie"; }
            else if (key.indexOf("AS7343_F") >= 0) { friendlyName = String("Spectral Channel ") + key.substring(7); icon = "mdi:palette"; }
            else if (key == "AS7343_Clear") { friendlyName = "Spectral Clear Channel"; icon = "mdi:palette-outline"; }
            else if (key == "AS7343_NIR") { friendlyName = "Spectral Near-IR Channel"; icon = "mdi:weather-sunny"; }
            else if (key == "AS7343_Flicker") { friendlyName = "Spectral Flicker Detection"; icon = "mdi:sine-wave"; }
            else if (key == "OPTICS_AS7343_Gain") { friendlyName = "AS7343 Gain Multiplier"; icon = "mdi:camera-iris"; }
            else if (key == "OPTICS_Sky_Ratio") { friendlyName = "Sky Blue Ratio"; icon = "mdi:cloud-percent"; }
            else if (key == "OPTICS_Sky_Status") { friendlyName = "Cloud Cover Status"; icon = "mdi:weather-partly-cloudy"; }
            else if (key == "OPTICS_PPFD") { friendlyName = "Photon Flux Density (PPFD)"; icon = "mdi:sprout"; }
            else if (key == "OPTICS_CCT") { friendlyName = "Color Temperature (CCT)"; icon = "mdi:lightbulb-on"; }
            else if (key == "VEML7700_Lux") { friendlyName = "Ambient Illuminance (Lux)"; icon = "mdi:brightness-5"; }
            else if (key == "VEML7700_White") { friendlyName = "White Light Level"; icon = "mdi:brightness-7"; }
            else if (key == "OPT4048_CIE_X") { friendlyName = "OPT4048 CIE X (Red)"; icon = "mdi:palette"; }
            else if (key == "OPT4048_CIE_Y") { friendlyName = "OPT4048 CIE Y (Green/Lux)"; icon = "mdi:palette"; }
            else if (key == "OPT4048_CIE_Z") { friendlyName = "OPT4048 CIE Z (Blue)"; icon = "mdi:palette"; }
            else if (key == "TCS34725_R") { friendlyName = "TCS34725 Red Channel"; icon = "mdi:palette"; }
            else if (key == "TCS34725_G") { friendlyName = "TCS34725 Green Channel"; icon = "mdi:palette"; }
            else if (key == "TCS34725_B") { friendlyName = "TCS34725 Blue Channel"; icon = "mdi:palette"; }
            else if (key == "TCS34725_C") { friendlyName = "TCS34725 Clear Channel"; icon = "mdi:palette-outline"; }
            else if (key == "TCS34725_ColorTemp_K") { friendlyName = "TCS34725 Color Temp"; icon = "mdi:thermometer"; }
            else if (key == "BMM350_Mag_X") { friendlyName = "BMM350 Magnetic X"; icon = "mdi:magnet"; }
            else if (key == "BMM350_Mag_Y") { friendlyName = "BMM350 Magnetic Y"; icon = "mdi:magnet"; }
            else if (key == "BMM350_Mag_Z") { friendlyName = "BMM350 Magnetic Z"; icon = "mdi:magnet"; }
            else if (key == "BMM350_Heading_Deg") { friendlyName = "Compass Heading"; icon = "mdi:compass"; }
            else if (key == "BMM350_Cardinal") { friendlyName = "Cardinal Direction"; icon = "mdi:compass-rose"; }
            // --- LSM6DSOX 6-axis IMU ---
            else if (key == "LSM6_Accel_X")       { friendlyName = "Acceleration X";            icon = "mdi:axis-x-arrow";       doc["unit_of_meas"] = "m/s²"; }
            else if (key == "LSM6_Accel_Y")       { friendlyName = "Acceleration Y";            icon = "mdi:axis-y-arrow";       doc["unit_of_meas"] = "m/s²"; }
            else if (key == "LSM6_Accel_Z")       { friendlyName = "Acceleration Z";            icon = "mdi:axis-z-arrow";       doc["unit_of_meas"] = "m/s²"; }
            else if (key == "LSM6_Accel_Mag")     { friendlyName = "Total Acceleration";        icon = "mdi:speedometer";        doc["unit_of_meas"] = "m/s²"; }
            else if (key == "LSM6_Gyro_X_rads")   { friendlyName = "Gyroscope X";               icon = "mdi:rotate-3d-variant";  doc["unit_of_meas"] = "rad/s"; }
            else if (key == "LSM6_Gyro_Y_rads")   { friendlyName = "Gyroscope Y";               icon = "mdi:rotate-3d-variant";  doc["unit_of_meas"] = "rad/s"; }
            else if (key == "LSM6_Gyro_Z_rads")   { friendlyName = "Gyroscope Z";               icon = "mdi:rotate-3d-variant";  doc["unit_of_meas"] = "rad/s"; }
            else if (key == "LSM6_Gyro_Mag_rads") { friendlyName = "Total Rotation Rate";       icon = "mdi:sync";               doc["unit_of_meas"] = "rad/s"; }
            else if (key == "LSM6_Temp_C")        { friendlyName = "IMU Die Temperature";       icon = "mdi:thermometer-chip";   doc["dev_cla"] = "temperature"; doc["unit_of_meas"] = "°C"; }
            else if (key == "LSM6_Tilt_Deg")      { friendlyName = "Tilt Angle";                icon = "mdi:angle-acute";        doc["unit_of_meas"] = "°"; }
            else if (key == "LSM6_Vibration")     { friendlyName = "Vibration Magnitude";       icon = "mdi:vibrate";            doc["unit_of_meas"] = "m/s²"; }
            else if (key == "LSM6_Shock_Det")     { friendlyName = "Shock Detected";            icon = "mdi:alert-rhombus"; }
            // --- LIS3MDL 3-axis Magnetometer ---
            else if (key == "LIS3MDL_X_uT")       { friendlyName = "Magnetic Field X";          icon = "mdi:magnet-on";          doc["unit_of_meas"] = "µT"; }
            else if (key == "LIS3MDL_Y_uT")       { friendlyName = "Magnetic Field Y";          icon = "mdi:magnet-on";          doc["unit_of_meas"] = "µT"; }
            else if (key == "LIS3MDL_Z_uT")       { friendlyName = "Magnetic Field Z";          icon = "mdi:magnet-on";          doc["unit_of_meas"] = "µT"; }
            else if (key == "LIS3MDL_Mag_uT")     { friendlyName = "Total Magnetic Field";      icon = "mdi:magnet";             doc["unit_of_meas"] = "µT"; }
            else if (key == "LIS3MDL_Heading")    { friendlyName = "IMU Compass Heading";       icon = "mdi:compass";            doc["unit_of_meas"] = "°"; }
            // ── NEW ADVANCED VIRTUAL SENSORS ──
            // Meteo
            else if (key == "METEO_Solar_GHI_Wm2")    { friendlyName = "Global Horiz. Irradiance"; icon = "mdi:solar-power";           doc["unit_of_meas"] = "W/m²"; }
            else if (key == "METEO_LCL_m")             { friendlyName = "Lifting Condensation Level";icon = "mdi:weather-cloudy-arrow";  doc["unit_of_meas"] = "m"; }
            else if (key == "METEO_ThetaE_K")          { friendlyName = "Equiv. Potential Temp θe"; icon = "mdi:thermometer-chevron-up";doc["unit_of_meas"] = "K"; }
            else if (key == "METEO_PW_mm")             { friendlyName = "Precipitable Water";        icon = "mdi:water-plus-outline";    doc["unit_of_meas"] = "mm"; }
            else if (key == "METEO_Conv_Instability")  { friendlyName = "Convective Instability";   icon = "mdi:weather-lightning";     doc["unit_of_meas"] = "/10"; }
            // Astro
            else if (key == "ASTRO_AOD")               { friendlyName = "Aerosol Optical Depth";    icon = "mdi:weather-hazy";          doc["unit_of_meas"] = ""; }
            // Space / Geomagnetic
            else if (key == "SPACE_K_Index_Proxy")     { friendlyName = "K-Index Proxy (local)";    icon = "mdi:magnet-on"; }
            else if (key == "SPACE_B_Total_uT")        { friendlyName = "Total Magnetic Field |B|"; icon = "mdi:magnet";                doc["unit_of_meas"] = "µT"; }
            else if (key == "SPACE_B_Baseline_uT")     { friendlyName = "Magnetic Quiet Baseline";  icon = "mdi:magnet-outline";        doc["unit_of_meas"] = "µT"; }
            else if (key == "SPACE_B_Dev_nT")          { friendlyName = "Geomagnetic Deviation";    icon = "mdi:chart-waterfall";       doc["unit_of_meas"] = "nT"; }
            else if (key == "SPACE_Aurora_Prob_Pct")   { friendlyName = "Aurora Probability";       icon = "mdi:weather-night";         doc["unit_of_meas"] = "%"; }
            else if (key == "SPACE_Aurora_Alert")      { friendlyName = "Aurora Alert";             icon = "mdi:alert-circle-outline"; }
            else if (key == "SPACE_Ozone_DU")          { friendlyName = "Ozone Column Proxy";       icon = "mdi:shield-sun-outline";    doc["unit_of_meas"] = "DU"; }
            // Medical
            else if (key == "MED_UTCI_C")              { friendlyName = "UTCI Thermal Climate Index"; icon = "mdi:human-greeting-proximity"; doc["dev_cla"] = "temperature"; doc["unit_of_meas"] = "°C"; }
            else if (key == "MED_UTCI_Cat")            { friendlyName = "UTCI Thermal Category";   icon = "mdi:tag-text-outline"; }
            else if (key == "MED_Required_ACH")        { friendlyName = "Required Ventilation ACH"; icon = "mdi:air-filter";            doc["unit_of_meas"] = "/h"; }
            else if (key == "MED_Radon_Risk")          { friendlyName = "Radon Accumulation Risk";  icon = "mdi:radioactive-circle-outline"; doc["unit_of_meas"] = "/10"; }
            // Geophysics
            else if (key == "GEOPHY_Seismic_Proxy")   { friendlyName = "Seismic/QVAR Proxy";       icon = "mdi:vibrate";               doc["unit_of_meas"] = "/10"; }
            // ── SPACE WEATHER BLOCK 2 ──
            else if (key == "SPACE_dBdt_nTs")          { friendlyName = "dB/dt CME Rate";          icon = "mdi:sine-wave";             doc["unit_of_meas"] = "nT/s"; }
            else if (key == "SPACE_SSC_Class")         { friendlyName = "Solar Storm Class (SSC)";  icon = "mdi:weather-lightning-rainy"; }
            else if (key == "SPACE_Forbush_Pct")       { friendlyName = "Forbush Decrease";         icon = "mdi:chart-line-variant";    doc["unit_of_meas"] = "%"; }
            else if (key == "SPACE_Forbush_Class")     { friendlyName = "Forbush Event Class";      icon = "mdi:radioactive"; }
            else if (key == "SPACE_Aurora_Sky_Clear")  { friendlyName = "Aurora Sky Window Clear";  icon = "mdi:weather-night"; }
            // ── MEDICAL BLOCK 2 ──
            else if (key == "MED_Lung_Deposit_Rest")   { friendlyName = "Lung PM Deposition (Rest)";icon = "mdi:lungs";                doc["unit_of_meas"] = "µg/min"; }
            else if (key == "MED_Lung_Deposit_Exer")   { friendlyName = "Lung PM Deposition (Exercise)";icon = "mdi:run-fast";         doc["unit_of_meas"] = "µg/min"; }
            // ── GAS SMOKE CLASSIFIER ──
            else if (key == "GAS_Smoke_Type")          { friendlyName = "Smoke Type Classifier";    icon = "mdi:smoke-detector"; }
            // ── ZMOD4510 NO2 + O3 ──
            else if (key == "ZMOD4510_NO2_ppb")        { friendlyName = "NO2 Concentration";        icon = "mdi:molecule";              doc["unit_of_meas"] = "ppb"; }
            else if (key == "ZMOD4510_O3_ppb")         { friendlyName = "O3 (Ozone) Concentration"; icon = "mdi:weather-sunny";         doc["unit_of_meas"] = "ppb"; }
            else if (key == "ZMOD4510_NO2_ugm3")       { friendlyName = "NO2 µg/m³";                icon = "mdi:molecule";              doc["unit_of_meas"] = "µg/m³"; }
            else if (key == "ZMOD4510_O3_ugm3")        { friendlyName = "O3 µg/m³";                 icon = "mdi:shield-sun";            doc["unit_of_meas"] = "µg/m³"; }
            else if (key == "ZMOD4510_FAST_AQI")       { friendlyName = "ZMOD Fast AQI (1-min)";    icon = "mdi:air-filter"; }
            else if (key == "ZMOD4510_EPA_AQI")        { friendlyName = "EPA AQI (O3 8hr)";         icon = "mdi:air-filter"; }
            else if (key == "ZMOD4510_O3_Risk")        { friendlyName = "Ozone Health Risk";        icon = "mdi:alert-circle"; }
            else if (key == "ZMOD4510_Smog_Index")     { friendlyName = "Summer Smog Index";        icon = "mdi:smog"; }
            else if (key == "ZMOD4510_Status")         { friendlyName = "ZMOD4510 Status";          icon = "mdi:progress-clock"; }
            // ── MLX90640 SKY ──
            else if (key == "MLX_Cloud_Cover_Pct")     { friendlyName = "Cloud Cover (IR)";         icon = "mdi:weather-cloudy";        doc["unit_of_meas"] = "%"; }
            else if (key == "MLX_Sky_Temp_C")          { friendlyName = "Sky Temperature (IR)";     icon = "mdi:thermometer-low";       doc["dev_cla"] = "temperature"; doc["unit_of_meas"] = "°C"; }
            else if (key == "MLX_Scene_Min_C")         { friendlyName = "IR Scene Min Temp";        icon = "mdi:thermometer-chevron-down"; doc["dev_cla"] = "temperature"; doc["unit_of_meas"] = "°C"; }
            else if (key == "MLX_Scene_Max_C")         { friendlyName = "IR Scene Max Temp";        icon = "mdi:thermometer-chevron-up";  doc["dev_cla"] = "temperature"; doc["unit_of_meas"] = "°C"; }
            else if (key == "MLX_Sky_Condition")       { friendlyName = "Sky Condition (IR)";       icon = "mdi:weather-partly-cloudy"; }
            // ── END NEW VIRTUAL SENSORS ──
            else if (key == "TCS34725_Lux") { friendlyName = "TCS34725 Ambient Lux"; icon = "mdi:brightness-5"; }
            else if (key == "VEML7700_ALS_Raw") { friendlyName = "Raw Ambient Light Sensor"; icon = "mdi:brightness-auto"; }
            else if (key == "LTR390_UVI") { friendlyName = "Ultraviolet Index (UVI)"; icon = "mdi:weather-sunny-alert"; }
            else if (key == "LTR390_UVS_Raw") { friendlyName = "Raw UV Sensor Data"; icon = "mdi:white-balance-sunny"; }
            else if (key == "LTR390_Burn_Time_min") { friendlyName = "Safe Sun Exposure Time"; icon = "mdi:timer-alert"; }
            else if (key == "AS3935_Strike_Distance_Km") { friendlyName = "Lightning Strike Distance"; icon = "mdi:flash-triangle"; }
            else if (key == "AS3935_Strike_Energy") { friendlyName = "Lightning Strike Energy"; icon = "mdi:lightning-bolt"; }
            else if (key == "AS3935_Strike_Count") { friendlyName = "Lightning Strike Count"; icon = "mdi:weather-lightning"; }
            else if (key == "Geiger_CPM") { friendlyName = "Radiation (Counts Per Minute)"; icon = "mdi:radioactive"; }
            else if (key == "Geiger_uSvh") { friendlyName = "Radiation Dose Rate"; icon = "mdi:radioactive-circle"; }
            else if (key == "Geiger_Pulses_Raw") { friendlyName = "Total Radiation Pulses"; icon = "mdi:counter"; }
            else if (key == "SHT45_Temp") { friendlyName = "SHT45 Temperature"; icon = "mdi:thermometer"; }
            else if (key == "SHT45_Hum") { friendlyName = "SHT45 Humidity"; icon = "mdi:water-percent"; }
            else if (key == "SGP41_Raw_VOC") { friendlyName = "SGP41 Raw VOC"; icon = "mdi:gas-cylinder"; }
            else if (key == "SGP41_Raw_NOx") { friendlyName = "SGP41 Raw NOx"; icon = "mdi:gas-cylinder"; }
            else if (key == "SGP41_VOC_Index") { friendlyName = "SGP41 VOC Index"; icon = "mdi:scent"; }
            else if (key == "SGP41_NOx_Index") { friendlyName = "SGP41 NOx Index"; icon = "mdi:smog"; }
            else if (key == "SGP41_VOC_Status") { friendlyName = "VOC Odor Status"; icon = "mdi:flower-tulip-outline"; }
            else if (key == "SGP41_NOx_Status") { friendlyName = "NOx Exhaust Status"; icon = "mdi:car-exhaust"; }
            else if (key == "TSL2591_Lux") { friendlyName = "Ambient Illuminance (TSL2591)"; icon = "mdi:brightness-5"; }
            else if (key == "TSL2591_Visible") { friendlyName = "TSL2591 Visible Light"; icon = "mdi:brightness-7"; }
            else if (key == "TSL2591_IR") { friendlyName = "TSL2591 Infrared"; icon = "mdi:weather-sunny"; }
            else if (key == "System_Mode") { friendlyName = "System Power Mode"; icon = "mdi:state-machine"; }
            else if (key == "System_Uptime_Hours") { friendlyName = "System Uptime"; icon = "mdi:clock-outline"; }
            else if (key == "System_Last_Fault_Hours_Ago") { friendlyName = "Last Hardware Fault"; icon = "mdi:history"; }
            else if (key.indexOf("LoadAvg") >= 0) { friendlyName = String("Hardware LoadAvg ") + key.substring(key.lastIndexOf("_") + 1) + "m"; icon = "mdi:chart-bell-curve-cumulative"; }
            else if (key.indexOf("Fault_") >= 0) { friendlyName = String("Hardware Fault: ") + key.substring(6); icon = "mdi:alert-decagram"; }

            // ===============================================================

            doc["name"] = String("AirSense ") + friendlyName;
            doc["stat_t"] = MQTT_TOPIC_STATE;
            doc["val_tpl"] = String("{{ value_json.sensors.") + key + " }}";
            doc["uniq_id"] = String(HOSTNAME) + "_" + key;
            if (icon != "") doc["icon"] = icon;
            
            JsonObject dev = doc["dev"].to<JsonObject>();
            dev["ids"][0] = HOSTNAME;
            dev["name"] = "A.T.L.A.S. Node";
            dev["mdl"] = "ESP32-S3 Station";
            dev["mf"] = "DevSpark";
            
            bool isMeasurement = true; 
            
            if (key.indexOf("Temp") >= 0) { doc["dev_cla"] = "temperature"; doc["unit_of_meas"] = "°C"; }
            else if (key.indexOf("Hum") >= 0 && key.indexOf("g_m3") == -1) { doc["dev_cla"] = "humidity"; doc["unit_of_meas"] = "%"; }
            else if (key.indexOf("Press_Raw") >= 0) { doc["dev_cla"] = "pressure"; doc["unit_of_meas"] = "Pa"; }
            else if (key.indexOf("Press") >= 0) { doc["dev_cla"] = "pressure"; doc["unit_of_meas"] = "hPa"; }
            else if (key.indexOf("PM") >= 0 && key.indexOf("Ratio") == -1 && key.indexOf("Num") == -1) { doc["dev_cla"] = "pm25"; doc["unit_of_meas"] = "µg/m³"; }
            else if (key.indexOf("Num_PM") >= 0) { doc["unit_of_meas"] = "pcs/cm³"; }
            else if (key.indexOf("Acc_mm") >= 0 || key.indexOf("Daily_Rain_mm") >= 0) { doc["dev_cla"] = "precipitation"; doc["unit_of_meas"] = "mm"; doc["stat_cla"] = "total_increasing"; }
            else if (key.indexOf("Intensity_mmph") >= 0) { doc["dev_cla"] = "precipitation_intensity"; doc["unit_of_meas"] = "mm/h"; doc["stat_cla"] = "measurement"; }
            else if (key.indexOf("Density") >= 0) { doc["unit_of_meas"] = "kg/m³"; }
            else if (key.indexOf("CO2") >= 0 && key.indexOf("eCO2") == -1) { doc["dev_cla"] = "carbon_dioxide"; doc["unit_of_meas"] = "ppm"; }
            else if (key.indexOf("Pct") >= 0 || key.indexOf("pct") >= 0) { doc["unit_of_meas"] = "%"; }
            else if (key.indexOf("ppb") >= 0) { doc["unit_of_meas"] = "ppb"; }
            else if (key.indexOf("ugm3") >= 0) { doc["unit_of_meas"] = "µg/m³"; }
            else if (key.indexOf("kPa") >= 0) { doc["dev_cla"] = "pressure"; doc["unit_of_meas"] = "kPa"; }
            else if (key.indexOf("Altitude_m") >= 0) { doc["unit_of_meas"] = "m"; }
            else if (key.indexOf("dP_dt") >= 0) { doc["unit_of_meas"] = "hPa/s"; }
            else if (key.indexOf("Delta") >= 0) { doc["unit_of_meas"] = "hPa/3h"; }
            else if (key.indexOf("QFE") >= 0) { doc["dev_cla"] = "pressure"; doc["unit_of_meas"] = "hPa"; }
            else if (key.indexOf("TAS_Proxy") >= 0) { doc["unit_of_meas"] = "kn"; }
            else if (key.indexOf("Class") >= 0 || key.indexOf("Risk") >= 0) { isMeasurement = false; }
            else if (key.indexOf("Base_m") >= 0) { doc["dev_cla"] = "distance"; doc["unit_of_meas"] = "m"; }
            else if (key.indexOf("Voltage") >= 0) { doc["dev_cla"] = "voltage"; doc["unit_of_meas"] = "V"; }
            else if (key.indexOf("Current") >= 0) { doc["dev_cla"] = "current"; doc["unit_of_meas"] = "mA"; }
            else if (key.indexOf("Power") >= 0) { doc["dev_cla"] = "power"; doc["unit_of_meas"] = "mW"; }
            else if (key.indexOf("Efficiency") >= 0 || key.indexOf("SOC") >= 0 || key == "BMV080_PM_Ratio_pct") { doc["dev_cla"] = "battery"; doc["unit_of_meas"] = "%"; }
            else if (key.indexOf("Lux") >= 0) { doc["dev_cla"] = "illuminance"; doc["unit_of_meas"] = "lx"; }
            else if (key.indexOf("eCO2") >= 0) { doc["dev_cla"] = "carbon_dioxide"; doc["unit_of_meas"] = "ppm"; }
            else if (key.indexOf("bVOC") >= 0) { doc["dev_cla"] = "volatile_organic_compounds_parts"; doc["unit_of_meas"] = "ppm"; }
            else if (key.indexOf("IAQ") >= 0 && key.indexOf("Accuracy") == -1 && key.indexOf("Status") == -1) { doc["dev_cla"] = "aqi"; }
            else if (key.indexOf("UVI") >= 0) { doc["dev_cla"] = "irradiance"; doc["unit_of_meas"] = "UV index"; }
            else if (key.indexOf("Distance") >= 0) { doc["dev_cla"] = "distance"; doc["unit_of_meas"] = "km"; }
            else if (key.indexOf("Rate") >= 0) { doc["icon"] = "mdi:battery-clock"; doc["unit_of_meas"] = "%/h"; }
            else if (key.indexOf("Trend") >= 0) { doc["dev_cla"] = "duration"; doc["unit_of_meas"] = "h"; }
            else if (key.indexOf("Res_Raw") >= 0) { doc["icon"] = "mdi:resistor"; doc["unit_of_meas"] = "Ω"; }
            else if (key.indexOf("min") >= 0) { doc["dev_cla"] = "duration"; doc["unit_of_meas"] = "min"; }
            else if (key.indexOf("mAh") >= 0) { doc["dev_cla"] = "energy_storage"; doc["unit_of_meas"] = "mAh"; }
            else if (key.indexOf("g_m3") >= 0) { doc["unit_of_meas"] = "g/m³"; }
            else if (key.indexOf("Visible") >= 0 || key.indexOf("IR") >= 0 || key.indexOf("Flicker") >= 0) { doc["unit_of_meas"] = "counts"; }
            else if (key.indexOf("Raw_VOC") >= 0 || key.indexOf("Raw_NOx") >= 0) { doc["unit_of_meas"] = "ticks"; }
            else if (key == "TCS34725_ColorTemp_K") { doc["dev_cla"] = "temperature"; doc["unit_of_meas"] = "K"; }
            else if (key.indexOf("Index") >= 0 || key.indexOf("Est_") >= 0) { doc["dev_cla"] = "aqi"; } 
            else if (key == "OPTICS_Sky_Ratio") { doc["unit_of_meas"] = "Ratio"; }
            else if (key == "OPTICS_PPFD") { doc["unit_of_meas"] = "μmol/m²/s"; }
            else if (key == "OPTICS_CCT") { doc["dev_cla"] = "temperature"; doc["unit_of_meas"] = "K"; }
            else if (key.indexOf("Gain") >= 0) { doc["unit_of_meas"] = "X"; }
            
            // --- FAULTS / BINARY SENSORS ---
            if (key.indexOf("Fault_") >= 0) {
                doc["dev_cla"] = "problem";
                isMeasurement = false;
            }
            
            if (key.indexOf("Obstructed") >= 0 || key.indexOf("Status") >= 0 || key.indexOf("Accuracy") >= 0 || key.indexOf("System_Mode") >= 0 || key.indexOf("Risk") >= 0) {
                isMeasurement = false;
            } else if (key.indexOf("Pulses") >= 0 || key.indexOf("Strike_Count") >= 0 || key.indexOf("Acc_mm") >= 0 || key.indexOf("Daily_Rain_mm") >= 0) {
                doc["stat_cla"] = "total_increasing";
                isMeasurement = false;
            } else if (key.indexOf("Intensity_mmph") >= 0) {
                doc["stat_cla"] = "measurement";
                isMeasurement = false;
            }
            
            if (isMeasurement) {
                doc["stat_cla"] = "measurement";
            }
            
            String out;
            serializeJson(doc, out);
            mqtt.publish(topic.c_str(), out.c_str(), true); 
            discovered_sensors.push_back(key);
        }
    }
    // --- HA DEVICE TRIGGER FOR MOBILE PUSH NOTIFICATIONS ---
    static bool trigger_discovered = false;
    if (!trigger_discovered) {
        String tTopic = String("homeassistant/device_automation/") + HOSTNAME + "/anomaly_detected/config";
        JsonDocument tDoc;
        tDoc["automation_type"] = "trigger";
        tDoc["type"] = "problem"; 
        tDoc["subtype"] = "detected";
        tDoc["payload"] = "ANOMALY_DETECTED";
        tDoc["topic"] = String("airsense/") + HOSTNAME + "/events";
        
        JsonObject dev = tDoc["device"].to<JsonObject>();
        dev["identifiers"][0] = HOSTNAME;
        dev["name"] = "A.T.L.A.S. Node";
        dev["model"] = "ESP32-S3 Station";
        dev["manufacturer"] = "DevSpark";
        
        String tOut; serializeJson(tDoc, tOut);
        mqtt.publish(tTopic.c_str(), tOut.c_str(), true);
        trigger_discovered = true;
    }

    if (payload["system"]["anomaly_active"] == true) {
        // Fire HA Device Trigger (allows easy visual binding in HA Automations)
        String eventTopic = String("airsense/") + HOSTNAME + "/events";
        mqtt.publish(eventTopic.c_str(), "ANOMALY_DETECTED");
        
        // Push detailed JSON payload for rich notifications
        JsonDocument notifyDoc;
        notifyDoc["title"] = "A.T.L.A.S. Threat Alert";
        notifyDoc["message"] = String("Rapid anomaly detected on: ") + payload["system"]["anomaly_desc"].as<String>();
        String notifyStr; serializeJson(notifyDoc, notifyStr);
        mqtt.publish("airsense/notify", notifyStr.c_str());
    }
    // --------------------------------------------------------
    JsonDocument notifyDoc;
        notifyDoc["title"] = "A.T.L.A.S. Threat Alert";
      
    String statePayload;
    serializeJson(payload, statePayload);
    mqtt.publish(MQTT_TOPIC_STATE, statePayload.c_str());
    mqtt.loop();
    ATLAS_LOG("[MQTT] Telemetry Payload successfully forwarded to Home Assistant!\n");
}

void pushDataAPI() {
    // WIFI AUTO-REPAIR przed próbą wysłania danych
    if (!repairWiFi()) {
        ATLAS_LOG("[API CLOUD] WiFi repair failed. Skipping API push but continuing execution.\n");
        return;
    }

    payload["node_id"] = HOSTNAME;
    
    if (currentMode == MODE_CONTINUOUS) payload["system_mode"] = "Continuous";
    else if (currentMode == MODE_LIGHT_SLEEP) payload["system_mode"] = "Light_Sleep";
    else if (currentMode == MODE_DEEP_SLEEP) payload["system_mode"] = "Deep_Sleep";
    else if (currentMode == MODE_MAINTENANCE) payload["system_mode"] = "Maintenance";
    else if (currentMode == MODE_RECOVERY) payload["system_mode"] = "Recovery";
    
    payload["timestamp_utc"] = time(nullptr); 
    
    pushMQTT();
    
    if (currentMode == MODE_MAINTENANCE || currentMode == MODE_RECOVERY) {
        ATLAS_LOG("\n[API CLOUD] Maintenance/Recovery mode active. Skipping public API push.\n");
        return; 
    }
    
    String jsonStr; serializeJson(payload, jsonStr);
    WiFiClientSecure client; client.setInsecure();
    HTTPClient http; http.begin(client, API_URL);
    http.addHeader("Content-Type", "application/json");
    
    ATLAS_LOG("\n[API CLOUD] Pushing telemetry package (%d bytes)...\n", jsonStr.length());
    int httpCode = http.POST(jsonStr);
    if(httpCode == 200 || httpCode == 201) ATLAS_LOG("[API CLOUD] Transaction Complete (HTTP %d).\n", httpCode);
    else ATLAS_LOG("[API CLOUD] Error Transmitting (HTTP %d)\n", httpCode);
    http.end();
}

void pushWeatherUnderground() {
    // WIFI AUTO-REPAIR przed próbą wysłania danych
    if (!repairWiFi()) {
        ATLAS_LOG("[API WU] WiFi repair failed. Skipping WU push but continuing execution.\n");
        return;
    }
    if(currentMode == MODE_MAINTENANCE || currentMode == MODE_RECOVERY) {
        ATLAS_LOG("[API WU] Skipping WU Push (Mode restriction or Offline)\n");
        return;
    }
    if(payload["sensors"]["ENV_Ext_Temp"].isNull() || payload["sensors"]["METEO_Sea_Level_Press_hPa"].isNull()) {
        ATLAS_LOG("[API WU] Skipping WU Push (Missing critical weather data)\n");
        return;
    }

    float tempC = payload["sensors"]["ENV_Ext_Temp"];
    float tempF = (tempC * 1.8f) + 32.0f;
    float pressHpa = payload["sensors"]["METEO_Sea_Level_Press_hPa"];
    float pressInHg = pressHpa * 0.02953f;
    float humidity = payload["sensors"]["ENV_Ext_Hum"];
    float dewC = tempC - ((100.0 - humidity) / 5.0);
    float dewF = (dewC * 1.8f) + 32.0f;
    
    String wuUrl = "http://rtupdate.wunderground.com/weatherstation/updateweatherstation.php?";
    wuUrl += "ID=" + String(WU_STATION_ID) + "&PASSWORD=" + String(WU_STATION_KEY) + "&dateutc=now";
    wuUrl += "&tempf=" + String(tempF, 1) + "&baromin=" + String(pressInHg, 2);
    wuUrl += "&humidity=" + String(humidity, 0) + "&dewptf=" + String(dewF, 1);
    
    if (!payload["sensors"]["BMV080_PM2_5"].isNull()) {
        wuUrl += "&AqPM2.5=" + String((float)payload["sensors"]["BMV080_PM2_5"], 1);
        wuUrl += "&AqPM10=" + String((float)payload["sensors"]["BMV080_PM10_0"], 1);
    }
    
    if (!payload["sensors"]["RG15_Daily_Rain_mm"].isNull()) {
        float daily_in = (float)payload["sensors"]["RG15_Daily_Rain_mm"] * 0.0393701f;
        float rate_in = (float)payload["sensors"]["RG15_Intensity_mmph"] * 0.0393701f;
        wuUrl += "&dailyrainin=" + String(daily_in, 2) + "&rainin=" + String(rate_in, 2);
    }
    
    if (!payload["sensors"]["WIND_Speed_Kph"].isNull()) {
        wuUrl += "&windspdmph=" + String((float)payload["sensors"]["WIND_Speed_Kph"] * 0.621371f, 1);
        wuUrl += "&winddir=" + String((int)payload["sensors"]["WIND_Direction_Deg"]);
    }
    
    if (!payload["sensors"]["WIND_Gust_Kph"].isNull()) {
        wuUrl += "&windgustmph=" + String((float)payload["sensors"]["WIND_Gust_Kph"] * 0.621371f, 1);
    }
    
    if (!payload["sensors"]["LTR390_UVI"].isNull()) {
        wuUrl += "&UV=" + String((float)payload["sensors"]["LTR390_UVI"], 1);
    }
    
    wuUrl += "&action=updateraw";

    HTTPClient http; http.begin(wuUrl); int httpCode = http.GET();
    if (httpCode > 0) ATLAS_LOG("[API WU] Weather Underground Sync Complete (HTTP %d)\n", httpCode);
    else ATLAS_LOG("[API WU] Failed to sync with WU (Error: %s)\n", http.errorToString(httpCode).c_str());
    http.end();
}

void fetchWindFromWU() {
    // WIFI AUTO-REPAIR przed próbą pobrania danych
    if (!repairWiFi()) {
        ATLAS_LOG("[API WU] WiFi repair failed. Skipping wind fetch but continuing execution.\n");
        return;
    }
    if (millis() - wu_wind_last_fetch < 300000) return;
    
    String wuUrl = "https://api.weather.com/v2/pws/obs/latest/" + String(WU_STATION_ID) + "/json?apiKey=" + String(WU_STATION_KEY);
    
    HTTPClient http2; http2.begin(wuUrl); 
    http2.setTimeout(5000);
    int httpCode = http2.GET();
    
    if (httpCode == 200) {
        String response = http2.getString();
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, response);
        
        if (!error) {
            wu_wind_speed = doc["observations"][0]["metric"]["windSpeed"].as<float>();
            wu_wind_dir = doc["observations"][0]["winddir"]["avg"].as<float>();
            wu_wind_gust = doc["observations"][0]["metric"]["windGust"].as<float>();
            
            payload["sensors"]["WIND_Speed_Kph"] = wu_wind_speed;
            payload["sensors"]["WIND_Direction_Deg"] = (int)wu_wind_dir;
            payload["sensors"]["WIND_Gust_Kph"] = wu_wind_gust;
            wu_wind_fetched = true;
            wu_wind_last_fetch = millis();
            ATLAS_LOG("[API WU] Wind: %.1f Kph @ %d deg (Gust: %.1f)\n", wu_wind_speed, (int)wu_wind_dir, wu_wind_gust);
        }
    }
    http2.end();
}

// ======================================================================
// WEATHERCLOUD INTEGRATION
// ======================================================================
void pushWeathercloud() {
    // WIFI AUTO-REPAIR przed próbą wysłania danych
    if (!repairWiFi()) {
        ATLAS_LOG("[API WC] WiFi repair failed. Skipping Weathercloud push but continuing execution.\n");
        return;
    }
    if(currentMode == MODE_MAINTENANCE || currentMode == MODE_RECOVERY) {
        ATLAS_LOG("[API WC] Skipping Weathercloud Push (Mode restriction or Offline)\n");
        return;
    }
    
    String wc_wid = "b978ed547c0fa045"; 
    String wc_key = "62377a66fa73d997b9d39249364f80e0";

    String url = "http://api.weathercloud.net/set?wid=" + wc_wid + "&key=" + wc_key;

    if (!payload["sensors"]["ENV_Ext_Temp"].isNull()) {
        url += "&temp=" + String((int)round((float)payload["sensors"]["ENV_Ext_Temp"] * 10.0f));
        url += "&hum=" + String((int)round((float)payload["sensors"]["ENV_Ext_Hum"]));
		// UWAGA!
		// KURWA NOTKA DLA MNIE
		// ADMIN AWEKAS PRZYJEBAL SIE ZE WYSYLAM
		// cisnienie, ktore nie jest sprowadzone do poziomu morza
		// poprawka dla calego systemu
        url += "&bar=" + String((int)round((float)payload["sensors"]["METEO_Sea_Level_Press_hPa"] * 10.0f));
        url += "&dew=" + String((int)round((float)payload["sensors"]["METEO_Dew_Point_C"] * 10.0f));
    }

    if (!payload["sensors"]["SHT45_Temp"].isNull()) {
        url += "&tempin=" + String((int)round((float)payload["sensors"]["SHT45_Temp"] * 10.0f));
        url += "&humin=" + String((int)round((float)payload["sensors"]["SHT45_Hum"]));
    }

    if (!payload["sensors"]["BMV080_PM2_5"].isNull()) {
        url += "&pm25=" + String((int)round((float)payload["sensors"]["BMV080_PM2_5"]));
        url += "&pm10=" + String((int)round((float)payload["sensors"]["BMV080_PM10_0"]));
    }

    if (!payload["sensors"]["LTR390_UVI"].isNull()) {
        url += "&uvi=" + String((float)payload["sensors"]["LTR390_UVI"], 1);
    }
    if (!payload["sensors"]["RG15_Daily_Rain_mm"].isNull()) {
        float daily_in = (float)payload["sensors"]["RG15_Daily_Rain_mm"] * 0.0393701f;
        float rate_in = (float)payload["sensors"]["RG15_Intensity_mmph"] * 0.0393701f;
        url += "&dailyrainin=" + String(daily_in, 2) + "&rainin=" + String(rate_in, 2);
    }
    if (!payload["sensors"]["TSL2591_Lux"].isNull()) {
        url += "&solarradiation=" + String((int)round(((float)payload["sensors"]["TSL2591_Lux"] / 120.0f) * 10.0f));
    }
    
    if (!payload["sensors"]["BMS_Cell_Voltage"].isNull()) {
        url += "&bat=" + String((int)round((float)payload["sensors"]["BMS_Cell_Voltage"] * 10.0f));
    }

    ATLAS_LOG("[API WC] URL: %s\n", url.c_str());

    WiFiClient client;
    HTTPClient http;
    http.begin(client, url);
    int httpCode = http.GET();
    
    if (httpCode > 0) {
        String response = http.getString();
        ATLAS_LOG("[API WC] Response (%d): %s\n", httpCode, response.c_str());
    } else {
        ATLAS_LOG("[API WC] HTTP GET Failed: %s\n", http.errorToString(httpCode).c_str());
    }
    http.end();
}


// ======================================================================
// AWEKAS INTEGRATION
// ======================================================================
void pushAwekas() {
    // WIFI AUTO-REPAIR przed próbą wysłania danych
    if (!repairWiFi()) {
        ATLAS_LOG("[API AWEKAS] WiFi repair failed. Skipping Awekas push but continuing execution.\n");
        return;
    }

    if(currentMode == MODE_MAINTENANCE || currentMode == MODE_RECOVERY) {
        ATLAS_LOG("[API AWEKAS] Skipping Awekas Push (Mode restriction or Offline)\n");
        return;
    }

    String awekasUser = "silasmariusz";
    String awekasPass = "axpl1029al"; // Zostawiam Twoje poprawne hasło

    String url = "http://ws.awekas.at/weatherstation/updateweatherstation.php?";
    url += "ID=" + awekasUser + "&PASSWORD=" + awekasPass + "&dateutc=now";

    if (!payload["sensors"]["ENV_Ext_Temp"].isNull()) {
        url += "&tempf=" + String(((float)payload["sensors"]["ENV_Ext_Temp"] * 1.8f) + 32.0f, 1);
        url += "&humidity=" + String((float)payload["sensors"]["ENV_Ext_Hum"], 0);
		// UWAGA!
		// KURWA NOTKA DLA MNIE
		// ADMIN AWEKAS PRZYJEBAL SIE ZE WYSYLAM
		// cisnienie, ktore nie jest sprowadzone do poziomu morza
		// poprawka dla calego systemu
        url += "&baromin=" + String((float)payload["sensors"]["METEO_Sea_Level_Press_hPa"] * 0.02953f, 2);
    }
    if (!payload["sensors"]["BME688_Comp_Temp"].isNull()) {
        url += "&extraTemp1=" + String(((float)payload["sensors"]["BME688_Comp_Temp"] * 1.8f) + 32.0f, 1);
        url += "&extraHum1=" + String((float)payload["sensors"]["BME688_Comp_Hum"], 0);
    }
    if (!payload["sensors"]["SHT45_Temp"].isNull()) {
        url += "&extraTemp2=" + String(((float)payload["sensors"]["SHT45_Temp"] * 1.8f) + 32.0f, 1);
        url += "&extraHum2=" + String((float)payload["sensors"]["SHT45_Hum"], 0);
    }
    if (!payload["sensors"]["LTR390_UVI"].isNull()) {
        url += "&UV=" + String((float)payload["sensors"]["LTR390_UVI"], 1);
    }
    if (!payload["sensors"]["RG15_Daily_Rain_mm"].isNull()) {
        float daily_in = (float)payload["sensors"]["RG15_Daily_Rain_mm"] * 0.0393701f;
        float rate_in = (float)payload["sensors"]["RG15_Intensity_mmph"] * 0.0393701f;
        url += "&dailyrainin=" + String(daily_in, 2) + "&rainin=" + String(rate_in, 2);
    }
    if (!payload["sensors"]["TSL2591_Lux"].isNull()) {
        url += "&solarradiation=" + String((int)round(((float)payload["sensors"]["TSL2591_Lux"] / 120.0f) * 10.0f));
    }
    url += "&action=updateraw";

    ATLAS_LOG("[API AWEKAS] URL: %s\n", url.c_str());

    WiFiClient client;
    HTTPClient http;
    http.begin(client, url);
    int httpCode = http.GET();
    
    if (httpCode > 0) {
        String response = http.getString();
        ATLAS_LOG("[API AWEKAS] Response (%d): %s\n", httpCode, response.c_str());
    } else {
        ATLAS_LOG("[API AWEKAS] HTTP GET Failed: %s\n", http.errorToString(httpCode).c_str());
    }
    http.end();
}

void printAsciiTable() {
    auto getV = [](const char* key, const char* unit, int prec = 2) -> String {
        if (payload["sensors"][key].isNull()) return "--" + String(unit);
        float v = payload["sensors"][key];
        return String(v, prec) + String(unit);
    };

    auto getS = [](const char* key) -> String {
        if (payload["sensors"][key].isNull()) return "N/A";
        return payload["sensors"][key].as<String>();
    };

    ATLAS_LOG("\n.------------------------------------------------------------------------------------------------------.\n");
    ATLAS_LOG("|                                  A.T.L.A.S.  SYSTEM  DASHBOARD (v2.2)                                |\n");
    ATLAS_LOG("|------------------------------------------------------------------------------------------------------|\n");
    
    char row[128];

    // --- ENVIRONMENT & CLIMATE ---
    ATLAS_LOG("| [ ENVIRONMENT & CLIMATE ]                                [ AIR QUALITY & GAS AI ]                    |\n");
    snprintf(row, sizeof(row), "| MS8607: %-8s %-7s %-10s | BME688: IAQ:%-5s %-10s %-10s |\n",
        getV("ENV_Ext_Temp", "C").c_str(), getV("ENV_Ext_Hum", "%", 1).c_str(), getV("ENV_Ext_Press", "hPa", 1).c_str(),
        getV("BME688_IAQ", "", 0).c_str(), getV("BME688_eCO2", "ppm", 0).c_str(), getV("BME688_bVOC", "ppm").c_str());
    ATLAS_LOG(row);
    
    snprintf(row, sizeof(row), "| SHT45 : %-8s %-7s              | SGP41 : VOC:%-5s NOx:%-5s                     |\n",
        getV("SHT45_Temp", "C").c_str(), getV("SHT45_Hum", "%", 1).c_str(),
        getV("SGP41_VOC_Index", "", 0).c_str(), getV("SGP41_NOx_Index", "", 0).c_str());
    ATLAS_LOG(row);

    snprintf(row, sizeof(row), "| BME280: %-8s %-7s %-10s |                                             |\n",
        getV("BME280_Enc_Temp", "C").c_str(), getV("BME280_Enc_Hum", "%", 1).c_str(), getV("BME280_Enc_Press", "hPa", 1).c_str());
    ATLAS_LOG(row);

    snprintf(row, sizeof(row), "| BMP585: %-10s %-12s      | SCD41 : CO2:%-7s %-7s %-7s      |\n",
        getV("BMP585_Pressure_hPa", "hPa", 1).c_str(), getS("BMP585_Trend").c_str(),
        getV("SCD41_CO2_ppm", "ppm", 0).c_str(), getV("SCD41_Temp", "C", 1).c_str(), getV("SCD41_Hum", "%", 1).c_str());
    ATLAS_LOG(row);

    snprintf(row, sizeof(row), "| ILPS  : %-10s QVAR:%-9s      | LIGHTNING: Str:%-3s Dist:%-5s             |\n",
        getV("ILPS22QS_Press_hPa", "hPa", 1).c_str(), getV("ILPS22QS_QVAR_mV", "mV", 1).c_str(),
        getV("AS3935_Strike_Count", "", 0).c_str(), getV("AS3935_Strike_Distance_Km", "km", 0).c_str());
    ATLAS_LOG(row);

    ATLAS_LOG("|------------------------------------------+-----------------------------------------------------------|\n");

    // --- LIGHT & RADIATION ---
    ATLAS_LOG("| [ OPTICS & RADIATION ]                   | [ PARTICULATES & RAIN ]                                   |\n");
    snprintf(row, sizeof(row), "| TSL2591: %-11s VEML: %-11s | BMV080: PM2.5:%-7s PM10:%-7s Ratio:%-5s |\n",
        getV("TSL2591_Lux", "Lx", 1).c_str(), getV("VEML7700_Lux", "Lx", 1).c_str(),
        getV("BMV080_PM2_5", "ug", 1).c_str(), getV("BMV080_PM10_0", "ug", 1).c_str(), getV("BMV080_PM_Ratio_pct", "%", 0).c_str());
    ATLAS_LOG(row);

    snprintf(row, sizeof(row), "| LTR390 : UVI:%-6s AS7331: UVI:%-6s | RG-15 : Acc: %-7s Int: %-7s               |\n",
        getV("LTR390_UVI", "", 1).c_str(), getV("AS7331_Medical_UVI", "", 1).c_str(),
        getV("RG15_EventAcc_mm", "mm", 2).c_str(), getV("RG15_Intensity_mmph", "mm/h", 1).c_str());
    ATLAS_LOG(row);

    snprintf(row, sizeof(row), "| GEIGER : %-9s %-12s      | AS7343: Clear:%-7s PPFD:%-7s CCT:%-6s |\n",
        getV("Geiger_uSvh", "uSv/h", 4).c_str(), getV("Geiger_CPM", "CPM", 1).c_str(),
        getV("AS7343_Clear", "ct", 0).c_str(), getV("OPTICS_PPFD", "umol", 2).c_str(), getV("OPTICS_CCT", "K", 0).c_str());
    ATLAS_LOG(row);

    ATLAS_LOG("|------------------------------------------+-----------------------------------------------------------|\n");

    // --- POWER & SYSTEM ---
    ATLAS_LOG("| [ POWER & BMS ]                                          [ CALCULATED BIOMETEO ]                     |\n");
    snprintf(row, sizeof(row), "| BATT: %-8s %-7s Rate:%-8s | SLP: %-10s  DewP: %-9s  VPD: %-8s |\n",
        getV("BMS_Cell_Voltage", "V").c_str(), getV("BMS_State_Of_Charge", "%", 1).c_str(), getV("BMS_Charge_Rate", "%/h", 2).c_str(),
        getV("METEO_Sea_Level_Press_hPa", "hPa", 1).c_str(), getV("METEO_Dew_Point_C", "C").c_str(), getV("METEO_VPD_kPa", "kPa").c_str());
    ATLAS_LOG(row);

    snprintf(row, sizeof(row), "| SOLAR: %-10s ESP Load:%-8s | Cloud:%-9s  AirDens:%-8s  WBulb:%-7s |\n",
        getV("SOLAR_Power_mW", "mW", 0).c_str(), getV("BMS_ESP_Current_mA", "mA", 1).c_str(),
        getV("METEO_Cloud_Base_m", "m", 0).c_str(), getV("METEO_Air_Density", "kg", 3).c_str(), getV("METEO_Wet_Bulb_C", "C").c_str());
    ATLAS_LOG(row);

    ATLAS_LOG("|------------------------------------------------------------------------------------------------------|\n");
    snprintf(row, sizeof(row), "| STATUS: %-15s | Uptime: %-8s hrs | Last Fault: %-15s |\n",
        getS("System_Mode").c_str(), getV("System_Uptime_Hours", "", 2).c_str(), getV("System_Last_Fault_Hours_Ago", "h ago", 2).c_str());
    ATLAS_LOG(row);

    // --- HARDWARE FAULTS ---
    ATLAS_LOG("| FAULTS: ");
    const char* faults[] = {"BME280", "BMV080", "INA219", "MS8607", "MAX17048", "BME688", "SHT45", "SGP41", "TSL2591", "OPT4048", "TCS34725", "AS7343", "VEML7700", "LTR390", "AS3935", "SCD41", "BMP585", "ILPS22QS", "AS7331", "RG15", "MLX90640"};
    for (const char* f : faults) {
        String key = "Fault_" + String(f);
        if (payload["sensors"][key] == "ON") {
            ATLAS_LOG("["); ATLAS_LOG(f); ATLAS_LOG("] ");
        }
    }
    ATLAS_LOG("                                                                              |\n");
    ATLAS_LOG("'------------------------------------------------------------------------------------------------------'\n");
}

// -----------------------------------------------------------------------
// 8. MAIN LOOP - STATE MACHINE
// -----------------------------------------------------------------------

void loop() {
    esp_task_wdt_reset();
    if (WiFi.status() == WL_CONNECTED) { server.handleClient(); ArduinoOTA.handle(); }

    if (currentMode == MODE_RECOVERY) {
        static bool recovery_started = false;
        if (!recovery_started) {
            ATLAS_LOG("\n[!!! EMERGENCY PULL DOWN !!!]\n");
            ATLAS_LOG("System entered RECOVERY mode. All scheduled routines suspended.\n");
            ATLAS_LOG("Waiting for manual OTA or HTTP intervention...\n");
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

    unsigned long currentMillis = millis();
    time_t now = time(nullptr);
    struct tm* timeinfo = localtime(&now);

    switch(currentPhase) {
        case PHASE_WAKEUP: {
            ATLAS_LOG("\n================ [ NEW SENSOR CYCLE ] ================\n");
            setupWiFiAndOTA();
            initSensors(); 
			scd41_triggered = false;
            currentPhase = PHASE_WARMUP;
            break;
        }
        case PHASE_WARMUP: {
            if (!baseline_taken && max17048_ok && tcaselect(MUX_MAX17048, CH_MAX17048)) {
                float tmp_v = bat.cellVoltage();
                float tmp_rate = bat.chargeRate();

				// ---- tu jest kolejny glitch odczytow --->
				// [BUG_HW#0001]
				// UWAGA, PROBLEMY Z GND, SPRAWDZIC GND
				// ZASTANOWIC SIE NAD IMPLEMENTACJA FILTRU WYCINAJACEGO NIECHCIANE 
				// PEAKI JAKO WORKAROUND SOFTWARE'owy, ale ostroznie
				// TO TYLKO MASKOWANIE PROBLEMU NIE JEGO ROZWIAZANIE
                // TARCZA 1: Sprawdzamy czy próbka tła nie jest z kosmosu
                if (!isnan(tmp_v) && tmp_v >= 2.0f && tmp_v <= 5.0f) {
                    baseline_voltage = tmp_v;
                    baseline_charge_rate = (tmp_rate > 50.0f || tmp_rate < -50.0f) ? 0.0f : tmp_rate;
                    baseline_taken = true;
                    ATLAS_LOG("[POWER] Tlo uspienia zarejestrowane: %.2fV, Rate: %.2f%%/hr\n", baseline_voltage, baseline_charge_rate);
                } else {
                    ATLAS_LOG("\n[!!! GLITCH GUARD] Odrzucono anomalię tła (%.2fV). Ponawiam...\n", tmp_v);
                }
            }
			
			if (!scd41_triggered && scd41_ok && tcaselect(MUX_SCD41, CH_SCD41)) {
                scd41.measureSingleShot();
                scd41_triggered = true;
            }

            static bool bmvFlushed = false;
            if (!bmvFlushed && (currentMode == MODE_LIGHT_SLEEP || currentMode == MODE_DEEP_SLEEP) && bmv080_ok && tcaselect(MUX_BMV080, CH_BMV080)) {
                Wire.beginTransmission(0x57);
                if (Wire.endTransmission() == 0) {
                    ATLAS_LOG("[SYSTEM] Oproznianie zbuforowanego pylu z lasera BMV080...\n");
                    int flushCount = 0;
                    bmv080_output_t rawOut;
                    while(bmv080.readSensor(&rawOut) && flushCount < 50) { flushCount++; delay(2); }
                }
                bmvFlushed = true;
            }

            static int last_sec = -1;
            int remaining = (dynamic_warmup_ms - (currentMillis - cycleStartTime)) / 1000;
            if (remaining != last_sec && remaining >= 0 && remaining % 5 == 0) {
                ATLAS_LOG("[SYSTEM] Warmup in progress... %d seconds remaining\n", remaining);
                last_sec = remaining;
            }

            if (currentMillis - lastBsecPollTime >= 50) {
                lastBsecPollTime = currentMillis;
                if (bme688_ok && tcaselect(MUX_BME688, CH_BME688)) {
                    if (!envSensor.run() && envSensor.status != BSEC_OK) {
                        checkBsecStatus(envSensor);
                    }
                }
            }

            if (currentMillis - lastFastPollTime >= 1000) {
                lastFastPollTime = currentMillis;
				
                // Wyzwalacz pomiaru NDIR dokładnie w 50 sekundzie minuty
                if (!scd41_triggered && scd41_ok && timeinfo->tm_sec == 50 && tcaselect(MUX_SCD41, CH_SCD41)) {
                    scd41.measureSingleShot(); scd41_triggered = true;
                }

                static int heartbeat_sec = 0;
                heartbeat_sec++;
                if (heartbeat_sec % 10 == 0) {
                    ATLAS_LOG("[SYSTEM] Continuous Mode: Waiting for the full minute to transmit (Current sec: %d)...\n", timeinfo->tm_sec);
                }

                if (bmv080_ok && tcaselect(MUX_BMV080, CH_BMV080)) {
                    Wire.beginTransmission(0x57);
                    if(Wire.endTransmission() == 0) {
                        bmv080_output_t rawOut;
                        if(bmv080.readSensor(&rawOut)) {
                            bmv_fail_counter = 0;
                            payload["sensors"]["BMV080_PM1_0"] = rawOut.pm1_mass_concentration;
                            payload["sensors"]["BMV080_PM2_5"] = rawOut.pm2_5_mass_concentration;
                            payload["sensors"]["BMV080_PM10_0"] = rawOut.pm10_mass_concentration;
                            payload["sensors"]["BMV080_Num_PM1_0"] = rawOut.pm1_number_concentration;
                            payload["sensors"]["BMV080_Num_PM2_5"] = rawOut.pm2_5_number_concentration;
                            payload["sensors"]["BMV080_Num_PM10_0"] = rawOut.pm10_number_concentration;
                            payload["sensors"]["BMV080_Obstructed"] = rawOut.is_obstructed ? 1 : 0;
                            payload["sensors"]["BMV080_Out_Of_Range"] = rawOut.is_outside_measurement_range ? 1 : 0;
                            if (rawOut.pm10_mass_concentration > 0) {
                                payload["sensors"]["BMV080_PM_Ratio_pct"] = (rawOut.pm2_5_mass_concentration / rawOut.pm10_mass_concentration) * 100.0f;
                            } else payload["sensors"]["BMV080_PM_Ratio_pct"] = 0.0f;
                        }
                    } else {
                        bmv_fail_counter++;
                        if (bmv_fail_counter > 15) {
                            ATLAS_LOG("[SYSTEM] BMV080 Timeout - No I2C ACK at 0x57. Marking offline.\n");
                            bmv080_ok = false;
                            bmv_fail_counter = 0;
                        }
                    }
                }
                
                if (sgp41_ok && tcaselect(MUX_SGP41, CH_SGP41)) {
                    uint16_t defaultRh = 0x8000;
                    uint16_t defaultT = 0x6666;  
                    if (sgp41.measureRawSignals(defaultRh, defaultT, current_sraw_voc, current_sraw_nox) == 0) {
                        current_voc_index = vocAlgorithm.process(current_sraw_voc);
                        current_nox_index = noxAlgorithm.process(current_sraw_nox);
                    }
                }
            }

            if(currentMillis - cycleStartTime >= dynamic_warmup_ms) {
                ATLAS_LOG("\n[SYSTEM] Hardware warmup complete. Activating Radio Uplink...\n");
                setupWiFiAndOTA(); 
                server.begin(); 
                
                if (currentMode == MODE_LIGHT_SLEEP || currentMode == MODE_DEEP_SLEEP) {
                    exhaustivelyReadSensors();
                    currentPhase = PHASE_PUSH_API;
                } else {
                    currentPhase = PHASE_READ;
                    lastReadTime = currentMillis;
                    bmvFlushed = false; 
                }
            }
            break;
        }
        case PHASE_READ: {
            if (currentMillis - lastBsecPollTime >= 50) {
                lastBsecPollTime = currentMillis;
                if (bme688_ok && tcaselect(MUX_BME688, CH_BME688)) {
                    if (!envSensor.run() && envSensor.status != BSEC_OK) {
                        checkBsecStatus(envSensor);
                    }
                }
            }

            if (currentMillis - lastFastPollTime >= 1000) {
                lastFastPollTime = currentMillis;
				
                // Wyzwalacz pomiaru NDIR dokładnie w 50 sekundzie minuty
                if (!scd41_triggered && scd41_ok && timeinfo->tm_sec == 50 && tcaselect(MUX_SCD41, CH_SCD41)) {
                    scd41.measureSingleShot(); scd41_triggered = true;
                }

                static int heartbeat_sec = 0;
                heartbeat_sec++;
                if (heartbeat_sec % 10 == 0) {
                    ATLAS_LOG("[SYSTEM] Continuous Mode: Waiting for the full minute to transmit (Current sec: %d)...\n", timeinfo->tm_sec);
                }

                if (bmv080_ok && tcaselect(MUX_BMV080, CH_BMV080)) {
                    Wire.beginTransmission(0x57);
                    if(Wire.endTransmission() == 0) {
                        bmv080_output_t rawOut;
                        if(bmv080.readSensor(&rawOut)) {
                            bmv_fail_counter = 0;
                            payload["sensors"]["BMV080_PM1_0"] = rawOut.pm1_mass_concentration;
                            payload["sensors"]["BMV080_PM2_5"] = rawOut.pm2_5_mass_concentration;
                            payload["sensors"]["BMV080_PM10_0"] = rawOut.pm10_mass_concentration;
                            payload["sensors"]["BMV080_Num_PM1_0"] = rawOut.pm1_number_concentration;
                            payload["sensors"]["BMV080_Num_PM2_5"] = rawOut.pm2_5_number_concentration;
                            payload["sensors"]["BMV080_Num_PM10_0"] = rawOut.pm10_number_concentration;
                            payload["sensors"]["BMV080_Obstructed"] = rawOut.is_obstructed ? 1 : 0;
                            payload["sensors"]["BMV080_Out_Of_Range"] = rawOut.is_outside_measurement_range ? 1 : 0;
                            if (rawOut.pm10_mass_concentration > 0) {
                                payload["sensors"]["BMV080_PM_Ratio_pct"] = (rawOut.pm2_5_mass_concentration / rawOut.pm10_mass_concentration) * 100.0f;
                            } else payload["sensors"]["BMV080_PM_Ratio_pct"] = 0.0f;
                        }
                    } else {
                        bmv_fail_counter++;
                        if (bmv_fail_counter > 15) {
                            ATLAS_LOG("[SYSTEM] BMV080 Timeout - No I2C ACK at 0x57. Marking offline.\n");
                            bmv080_ok = false;
                            bmv_fail_counter = 0;
                        }
                    }
                }
                
                if (sgp41_ok && tcaselect(MUX_SGP41, CH_SGP41)) {
                    uint16_t defaultRh = 0x8000;
                    uint16_t defaultT = 0x6666;  
                    if (sgp41.measureRawSignals(defaultRh, defaultT, current_sraw_voc, current_sraw_nox) == 0) {
                        current_voc_index = vocAlgorithm.process(current_sraw_voc);
                        current_nox_index = noxAlgorithm.process(current_sraw_nox);
                    }
                }
            }

            static int lastPushedMinute = -1;
            if (now > 1600000000 && timeinfo->tm_sec == 0 && timeinfo->tm_min != lastPushedMinute) {
                lastPushedMinute = timeinfo->tm_min;
                exhaustivelyReadSensors();
				scd41_triggered = false; // Resetujemy flagę na kolejną minutę
                currentPhase = PHASE_PUSH_API;
            }
            break;
        }
        case PHASE_PUSH_API: {
            if (WiFi.status() != WL_CONNECTED) {
                ATLAS_LOG("\n[SYSTEM] Detected connection drop before API push. Reconnecting...\n");
                setupWiFiAndOTA();
            }

            checkRemoteModeOverride();
            
            float current_soc = !payload["sensors"]["BMS_State_Of_Charge"].isNull() ? (float)payload["sensors"]["BMS_State_Of_Charge"] : 100.0f;
            
            if (currentMode == MODE_LIGHT_SLEEP) {
                bool force_deep = false;
                String deep_reason = "";
                
                if (ENABLE_NIGHT_AUTO_DEEP_SLEEP && now > 1600000000 && (timeinfo->tm_hour >= 23 || timeinfo->tm_hour < 6)) {
                    force_deep = true;
                    deep_reason = "Tryb Nocny (23:00-06:00)";
                }
                else if (max17048_ok && current_soc < 70.0f) {
                    force_deep = true;
                    deep_reason = String("Bateria spadła ponizej 70% (obecnie ") + String(current_soc, 1) + "%)";
                }

                if (force_deep) {
                    currentMode = MODE_DEEP_SLEEP;
                    ATLAS_LOG("\n[SYSTEM] %s. Automatyczne nadpisanie na DEEP SLEEP.\n", deep_reason.c_str());
                }
            }

            String finalModeName = "Continuous";
            if (currentMode == MODE_LIGHT_SLEEP) finalModeName = "Light Sleep";
            else if (currentMode == MODE_DEEP_SLEEP) finalModeName = "Deep Sleep";
            payload["sensors"]["System_Mode"] = finalModeName;
            pushDataAPI();
            fetchWindFromWU();
            pushWeatherUnderground();
            pushWeathercloud();
            pushAwekas();
            
            if (mqtt.connected()) {
                mqtt.disconnect();
            }
            
            if(currentMode == MODE_DEEP_SLEEP) {
                unsigned long sleepTimeMs;
                int interval_min = 5; 
                
               bool anomaly_active = false;
                if (!payload["system"]["anomaly_active"].isNull()) {
                    anomaly_active = payload["system"]["anomaly_active"];
                }
                
                if (anomaly_active && (!max17048_ok || current_soc >= 30.0f)) {
                    interval_min = 2; // High-resolution tracking mode
                    ATLAS_LOG("[POWER] Anomaly tracking active! Deep Sleep shortened to 2 minutes.\n");
                } else if (max17048_ok && current_soc < 30.0f) {
                if (max17048_ok && current_soc < 30.0f) {
                    interval_min = 60;
                    ATLAS_LOG("[POWER] SOC < 30%%. Deep Sleep wydluzony do 60 minut!\n");
                } else if (max17048_ok && current_soc < 50.0f) {
                    interval_min = 10;
                    ATLAS_LOG("[POWER] SOC < 50%%. Deep Sleep wydluzony do 10 minut.\n");
                }

                if (now > 1600000000) {
                    int current_min = timeinfo->tm_min;
                    int current_sec = timeinfo->tm_sec;
                    int next_target_min = ((current_min / interval_min) + 1) * interval_min;
                    int seconds_to_target = (next_target_min * 60) - (current_min * 60 + current_sec);
                    int sleep_sec = seconds_to_target - 45;
                    if (sleep_sec < 10) sleep_sec += (interval_min * 60); 
                    sleepTimeMs = sleep_sec * 1000ULL;
                } else {
                    sleepTimeMs = (interval_min * 60 - 45) * 1000ULL;
                }

                ATLAS_LOG("\n[POWER SYSTEM] Microcontroller entering DEEP SLEEP for: %lu ms.\n", sleepTimeMs);
                sleepSensors(true); 
                delay(800);
                esp_sleep_enable_timer_wakeup(sleepTimeMs * 1000ULL);
                esp_deep_sleep_start();
            } 
            else if(currentMode == MODE_LIGHT_SLEEP) {
                unsigned long sleepTimeMs;
                dynamic_warmup_ms = 12000;
                if (now > 1600000000) {
                    int current_sec = timeinfo->tm_sec;
                    int sleep_sec = 45 - current_sec;
                    if (sleep_sec <= 0) sleep_sec += 60;
                    sleepTimeMs = sleep_sec * 1000ULL;
                } else {
                    unsigned long activeTime = millis() - cycleStartTime;
                    sleepTimeMs = (activeTime < CYCLE_DURATION_MS) ? (CYCLE_DURATION_MS - activeTime) : 5000;
                }

                ATLAS_LOG("\n[POWER SYSTEM] Microcontroller entering LIGHT SLEEP for: %lu ms.\n", sleepTimeMs);
                sleepSensors(false); 
                WiFi.disconnect(true);
                WiFi.mode(WIFI_OFF);
                
                // --- ROZWIĄZANIE PROBLEMU W PĘTLI LIGHT SLEEP ---
                if (geiger_ok) {
                    updateGeigerPCNT();                 // Zbierz zaległe uderzenia PCNT z trybu Active
                    pcnt_counter_pause(PCNT_UNIT_0);    // SPauzuj sprzętowy licznik, aby zapobiec usterkom domeny zasilania APB przy uśpieniach
                }

                uint64_t target_us = esp_timer_get_time() + (sleepTimeMs * 1000ULL);
                gpio_wakeup_enable((gpio_num_t)GEIGER_IRQ_PIN, GPIO_INTR_LOW_LEVEL);
                esp_sleep_enable_gpio_wakeup();
                
                while(esp_timer_get_time() < target_us) {
                    uint64_t remaining_us = target_us - esp_timer_get_time();
                    esp_sleep_enable_timer_wakeup(remaining_us);
                    esp_light_sleep_start(); 
                    
                    if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_GPIO) {
                        // 1. Rejestrujemy impuls wybudzający (skoro PCNT spi)
                        geiger_pulses = geiger_pulses + 1; 
                        
                        // 2. CRITICAL FIX: Zablokowanie nieskończonej pętli usterek!
                        // Czekamy aż fizyczny impuls ulegnie zakończeniu
						// Bez tego, ESP 
						// wybudza i usypia się 100 000 razy na sekundę podbijając statystyki w kosmos.
                        while(digitalRead(GEIGER_IRQ_PIN) == LOW) {
                            delayMicroseconds(10);
                        }
                    }
                }
                gpio_wakeup_disable((gpio_num_t)GEIGER_IRQ_PIN);
                esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_GPIO);
                
                if (geiger_ok) {
                    pcnt_counter_clear(PCNT_UNIT_0);
                    pcnt_counter_resume(PCNT_UNIT_0); // Przywracamy sprzętowy licznik po wybudzeniu
                }
                // ------------------------------------------------

                ATLAS_LOG("\n[SYSTEM] Waking up from LIGHT SLEEP...\n");
                if(bme280_ok && tcaselect(MUX_BME280, CH_BME280)) bme280.setSampling(Adafruit_BME280::MODE_NORMAL);
                if(max17048_ok && tcaselect(MUX_MAX17048, CH_MAX17048)) { bat.sleep(false); bat.wake(); }
                if(tsl2591_ok && tcaselect(MUX_TSL2591, CH_TSL2591)) tsl2591.enable();
                
                if(veml_ok && tcaselect(MUX_VEML, CH_VEML)) veml.powerSaveEnable(false);

                // Wybudzenie laboratorium
                if(scd41_ok && tcaselect(MUX_SCD41, CH_SCD41)) { 
                    scd41.wakeUp(); 
                    delay(30); // <--- ZABEZPIECZENIE SPRZĘTOWE (SCD41 potrzebuje min. 20ms na start rejestrów)
                    scd41_triggered = false; 
                }

                cycleStartTime = millis();
                currentPhase = PHASE_SLEEP_WAIT; 
                payload.clear();
                geiger_pulses = 0; 
                baseline_taken = false;
            } else {
                cycleStartTime = millis();
                currentPhase = PHASE_SLEEP_WAIT; 
                payload.clear();
                geiger_pulses = 0; 
                baseline_taken = false;
            }
            break;
        }
        }
        case PHASE_SLEEP_WAIT: {
            int fail_count = 0;
            if (!bme280_ok) fail_count++;
            if (!bmv080_ok) fail_count++;
            if (!ms8607_ok) fail_count++;
            if (!max17048_ok) fail_count++;
            if (!ina219_ok) fail_count++;
            if (!sht45_ok) fail_count++;
            if (!sgp41_ok) fail_count++;
            if (!tsl2591_ok) fail_count++;
            if (!opt4048_ok) fail_count++;
            if (!tcs34725_ok) fail_count++;
            if (!i2c_mem_ok) fail_count++;
            if (!as7343_ok) fail_count++;
            if (!veml_ok) fail_count++;
            if (!ltr_ok) fail_count++;
            
            // --- TWARDY AUTO-HEALING MAGISTRALI ---
            if (fail_count >= 2) {
                ATLAS_LOG("\n[!!! ALARM] Krytyczna awaria I2C (%d urzadzen offline). Wymuszanie twardego resetu magistrali!\n", fail_count);
                Wire.end();
                delay(100);
                Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
                Wire.setClock(100000);
                delay(100);
            }
            
			// !!! 
			// AUTO HEALING
			// !!!
            if (fail_count > 0 || !bme688_ok || !as3935_ok || !geiger_ok) {
                ATLAS_LOG("\n[SYSTEM] Recovering disconnected sensors silently...\n");
                if(!i2c_mem_ok) initI2CMemory();
                if(!bme280_ok) initBME280();
                if(!bmv080_ok) initBMV080();
                if(!ms8607_ok) initMS8607();
                if(!max17048_ok) initMAX17048();
                if(!ina219_ok) initINA219();
                if(!sht45_ok) initSHT45();
                if(!sgp41_ok) initSGP41();
                if(!tsl2591_ok) initTSL2591();
                if(!opt4048_ok) initOPT4048();
                if(!tcs34725_ok) initTCS34725();
                
                if(!as7343_ok) initAS7343();
                if(!veml_ok) initVEML7700();
                if(!ltr_ok) initLTR390();
                if(!as3935_ok) initAS3935();
                if(!geiger_ok) initGeiger();
                
                // --- DODANE BRAKUJĄCE CZUJNIKI LABORATORYJNE ---
                if(!scd41_ok) initSCD41();
                if(!bmp585_ok) initBMP585();
                if(!ilps_ok) initILPS22QS();
                if(!as7331_ok) initAS7331();
                if(!rg15_ok) initRG15();
            }
            currentPhase = PHASE_WARMUP;
            break;
        }
    }
}