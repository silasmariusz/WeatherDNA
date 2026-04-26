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
#define DEBUG_LEVEL 2 // 0=silent, 1=errors, 2=verbose
#define ERROR_LED_ENABLED true

// --- MUX TOPOLOGY ---
const uint8_t MUX_POWER   = 0x70;
const uint8_t MUX_OPTICS  = 0x71;
const uint8_t MUX_WEATHER = 0x72;

// MUX 0x70 (Power/Seismic)
#define CH0_MAX17048 0
#define CH1_INA219   1
#define CH2_BME280   2
#define CH3_IMU      3 // LSM6DSOX + LIS3MDL
#define CH4_AS3935   4
#define CH6_OPT_SHARED 6 // TSL2591, OPT4048, TCS34725
#define CH7_BMM350_A 7

// MUX 0x71 (Optics)
#define CH0_AS7331   0
#define CH1_AS7343   1
#define CH2_MLX90640 2
#define CH3_VEML7700 3
#define CH4_LTR390   4
#define CH6_ILPS22QS 6
#define CH7_BMM350_B 7

// MUX 0x72 (Stevenson Screen)
#define CH0_BME688   0
#define CH1_ZMOD4510 1
#define CH2_GAS_NDIR 2 // SCD41 + SGP41
#define CH3_BMV080   3
#define CH4_BME690   4
#define CH5_SHT45    5
#define CH7_BMP585   7

#endif
