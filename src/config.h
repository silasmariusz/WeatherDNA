#ifndef CONFIG_H
#define CONFIG_H

// --- HARDWARE PINS ---
#define RGB_LED_PIN    48
#define I2C_SDA_PIN    4
#define I2C_SCL_PIN    5
#define GEIGER_PIN     5  // D5 for PCNT
#define RAIN_RG15_TX   43 // D6
#define RAIN_RG15_RX   44 // D7
#define AS3935_IRQ_PIN 7

// --- SYSTEM SETTINGS ---
#define HOSTNAME "WeatherDNA-ATLAS"
#define STATION_ALTITUDE 290.0f
#define STATIC_IP "10.100.200.17"
#define DEBUG_LEVEL 2
#define ERROR_LED_ENABLED true

// --- MUX TOPOLOGY ---
const uint8_t MUX_POWER   = 0x70;
const uint8_t MUX_OPTICS  = 0x71;
const uint8_t MUX_WEATHER = 0x72;

// I2C ADDR DATABASE FOR SCANNER
struct I2C_Dev { uint8_t addr; const char* name; };
const I2C_Dev addr_list[] = {
    {0x72, "MUX Stevenson (HUB)"}, {0x77, "BME688 AI (Stevenson)"}, {0x33, "ZMOD4510 (Stevenson)"},
    {0x62, "SCD41 NDIR (Stevenson)"}, {0x59, "SGP41 VOC/NOx (Stevenson)"}, {0x57, "BMV080 Dust (Stevenson)"},
    {0x44, "SHT45 Ref (Stevenson)"}, {0x46, "BMP585 Baro (Stevenson)"}, {0x50, "EEPROM (Main Bus)"},
    {0x70, "MUX Power (HUB)"}, {0x71, "MUX Optics (HUB)"}, {0x36, "MAX17048 Fuel"}, {0x40, "INA219 Solar"},
    {0x76, "BME280 BMS"}, {0x6A, "LSM6DSOX IMU"}, {0x1E, "LIS3MDL Mag"}, {0x14, "BMM350 Precise Mag"},
    {0x74, "AS7331 Med UV"}, {0x39, "AS7343 Spectral"}, {0x10, "VEML7700 Lux"}, {0x53, "LTR390 UV Index"},
    {0x5C, "ILPS22QS QVAR"}, {0, NULL}
};

#define CH0_MAX17048 0
#define CH1_INA219   1
#define CH2_BME280   2
#define CH3_IMU      3 
#define CH4_AS3935   4
#define CH6_OPT_SHARED 6 
#define CH7_BMM350_A 7
#define CH0_AS7331   0
#define CH1_AS7343   1
#define CH2_MLX90640 2
#define CH3_VEML7700 3
#define CH4_LTR390   4
#define CH6_ILPS22QS 6
#define CH7_BMM350_B 7
#define CH0_BME688   0
#define CH1_ZMOD4510 1
#define CH2_GAS_NDIR 2 
#define CH3_BMV080   3
#define CH4_BME690   4
#define CH5_SHT45    5
#define CH7_BMP585   7

#endif
