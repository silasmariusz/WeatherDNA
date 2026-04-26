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
/// DODAC BSEC (wersja v3.3)
#include <Adafruit_MS8607.h>   
#include "SparkFun_BMV080_Arduino_Library.h" 

// --- NEW SENSORS ---
#include "Adafruit_SHT4x.h"
#include <SensirionI2CSgp41.h>
#include <VOCGasIndexAlgorithm.h>
#include <NOxGasIndexAlgorithm.h>

// --- NEW LABORATORY SENSORS ---
#include <SensirionI2cScd4x.h>      // NDIR CO2
#include <Adafruit_BMP5xx.h>
#include <ILPS22QSSensor.h>         // ILPS22QST (STM32duino)

// ── ZMOD4510 Renesas SDK ── (NO2 + O3)
// Firmware SDK: D:\Arduino\libraries\Renesas-ZMOD4510-NO2_O3-Firmware
// precompiled=true; algo linked from lib/Espressif ESP/esp32s3/*.a
// HAL: built-in Arduino Wire HAL (hal/arduino/arduino.cpp)
#include "no2_o3-arduino.h"
#include "hal/arduino/arduino_hal.h"  // for HAL_Init()


void bme688Callback(const bme68xData data, const bsecOutputs outputs, Bsec2 bsec);

// --- FORWARD DECLARATIONS ---
bool tcaselect(uint8_t mux_addr, uint8_t i);

// --- DYNAMIC I2C MUX ROUTING ---
uint8_t MUX_BME280   = 0x70; uint8_t CH_BME280   = 2; // (0x70) - User: 0x76 @ CH2
uint8_t MUX_BMV080   = 0x72; uint8_t CH_BMV080   = 3; // (0x72)
uint8_t MUX_MS8607   = 0x72; uint8_t CH_MS8607   = 6; // (0x72) - [DISABLED]
uint8_t MUX_BME688   = 0x72; uint8_t CH_BME688   = 0; // BME688 AI Waveshare na CH0
uint8_t MUX_SHT45    = 0x72; uint8_t CH_SHT45    = 5; // (0x72)
uint8_t MUX_SGP41    = 0x72; uint8_t CH_SGP41    = 2; // (0x72) współdzielony z SCD41
uint8_t MUX_I2CMEM   = 0x00; uint8_t CH_I2CMEM   = 0; // Parallel on main bus
uint8_t MUX_SCD41    = 0x72; uint8_t CH_SCD41    = 2; // shared CH2
uint8_t MUX_BMP585   = 0x72; uint8_t CH_BMP585   = 7; // Pressure
uint8_t MUX_ILPS     = 0x71; uint8_t CH_ILPS     = 6; // shared CH6
// ZMOD4510 HAL is provided by the SDK's built-in Arduino HAL (hal/arduino/arduino.cpp)
// HAL_Init() populates Wire-based callbacks automatically.
uint8_t MUX_ZMOD4510 = 0x72; uint8_t CH_ZMOD4510 = 1; // (0x72) CH1 — I2C 0x33
#define AS3935_IRQ_PIN 7 // [XIAO: D8] - Pin RTC dla przerwań błyskawic

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

unsigned long lastReadTime = 0;
unsigned long lastFastPollTime = 0;
unsigned long lastBsecPollTime = 0;

Bsec2 envSensor;                     bool bme688_ok = false;
SparkFunBMV080 bmv080;               bool bmv080_ok = false;
Adafruit_MS8607 ms8607;              bool ms8607_ok = false;
DFRobot_AS3935_I2C lightning((uint8_t)AS3935_IRQ_PIN, (uint8_t)0x03); bool as3935_ok = false;
Adafruit_BME280 bme280;              bool bme280_ok = false;
Adafruit_SHT4x sht45;                bool sht45_ok = false;
SensirionI2CSgp41 sgp41;             bool sgp41_ok = false;
VOCGasIndexAlgorithm vocAlgorithm;
NOxGasIndexAlgorithm noxAlgorithm;
bool i2c_mem_ok = false;
SensirionI2cScd4x scd41;             bool scd41_ok = false; bool scd41_triggered = false;
Adafruit_BMP5xx bmp585;              bool bmp585_ok = false;
ILPS22QSSensor ilps(&Wire);          bool ilps_ok = false;
static zmod4xxx_dev_t   zmod4510_dev;
static uint8_t          zmod4510_adc[ZMOD4510_ADC_DATA_LEN];
static uint8_t          zmod4510_prod[ZMOD4510_PROD_DATA_LEN];
static no2_o3_handle_t  zmod4510_algo_handle;
static no2_o3_results_t zmod4510_results;
static no2_o3_inputs_t  zmod4510_input;
static Interface_t      zmod4510_hal;
bool zmod4510_ok = false;
bool zmod4510_stabilizing = false;

// --- RTC MEMORY FOR ALGORITHMS ---
RTC_DATA_ATTR float last_pressure_bmp585 = 0.0f;
...

float wu_wind_speed = 0.0f;
float wu_wind_dir = 0.0f;
float wu_wind_gust = 0.0f;
bool wu_wind_fetched = false;
unsigned long wu_wind_last_fetch = 0;


...
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

const float STATION_ALTITUDE_METERS = 290.0f;

// ─────────────────────────────────────────────────────────────────────────────
// Station constants used across multiple formulas
// Nowy Sącz, PL:  geographic lat 49.62°N
// Geomagnetic lat ≈ geographic + 11.5° → ~61.1°N effective
// ─────────────────────────────────────────────────────────────────────────────
static const float STATION_GEO_LAT   = 49.62f;
static const float STATION_GEOMAG_LAT = 61.1f; // for aurora calculations

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

bool initMS8607() {
    if(tcaselect(MUX_MS8607, CH_MS8607)) {
        if(ms8607.begin()) { 
            ATLAS_LOG("[HW INIT] CH3: MS8607 Precision PHT -> ONLINE\n");
            ms8607_ok = true; return true;
        }
    }
    ms8607_ok = false; return false;
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


bool initI2CMemory() {
    // Parallel on main bus - no tcaselect needed
    Wire.beginTransmission(0x50);
    if(Wire.endTransmission() == 0) {
        ATLAS_LOG("[HW INIT] BUS: I2C Memory (0x50) -> ONLINE (AI Baseline Storage)\n");
        i2c_mem_ok = true; return true;
    }
    i2c_mem_ok = false; return false;
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
    if(!ms8607_ok) initMS8607();
    if(!bme688_ok) initBME688();
    if(!sht45_ok) initSHT45();
    if(!sgp41_ok) initSGP41();
    if(!scd41_ok) initSCD41();
    if(!bmp585_ok) initBMP585();
    if(!ilps_ok) initILPS22QS();
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


    if(i2c_mem_ok && tcaselect(MUX_I2CMEM, CH_I2CMEM)) {
            Wire.beginTransmission(0x50);
            if(Wire.endTransmission() != 0) i2c_mem_ok = false;
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
...