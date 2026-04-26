# SKILL: WeatherDNA Project

## 1. Hardware Configuration
*   **Board**: ESP32-S3 DevKitC-1 (WROOM-1 N16R8)
*   **Memory**: 16MB FLASH / 8MB PSRAM
*   **Bus Config**:
    *   **I2C SDA**: GPIO 4 (D3)
    *   **I2C SCL**: GPIO 5 (D4)
    *   **UART TX**: GPIO 43 (D6)
    *   **UART RX**: GPIO 44 (D7)

## 2. Stevenson Screen Topology (MUX 0x72)
*   **CH0** | BME688 AI (0x77)
*   **CH1** | ZMOD4510 (0x33)
*   **CH2** | SCD41 (0x62) + SGP41 (0x59)
*   **CH3** | BMV080 (0x57)
*   **CH4** | BME690 (0x77)
*   **CH5** | SHT45 (0x44)
*   **CH6** | ILPS22QS (0x5C)
*   **CH7** | BMP585 (0x46)

## 3. Operational Logic
*   **Mode**: Continuous (Active WiFi)
*   **Calculations**: Minimal (calcSLP only)
