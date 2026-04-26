# AirSense2 / WeatherDNA Project - Agent Instructions

This project is a high-end environmental monitoring system ("ATLAS Environmental OS") based on the ESP32-S3. It uses a triple-MUX I2C topology and Bosch BSEC3 AI for professional-grade air quality analytics.

## 🛠️ Mandatory Architectural Pacts (SKILL.md Compliance)

### 1. I2C MUX Hygiene (Rule 4.1)
All sensors are behind TCA9548A multiplexers. 
- **NAKAZ:** Every `muxSelect()` call must be followed by a **5ms delay** to stabilize bus capacitance.
- **NAKAZ:** The `main.cpp` wrapper `muxSelect()` implements this automatically. Use it.

### 2. BSEC3 AI Engine (Rule 7)
The Bosch BSEC3 library manages the BME690.
- **Timing:** BSEC3 is time-critical. The `loop()` must prioritize `runBSEC3()` and respect `next_call`.
- **Persistence:** AI learning states (BaseLine) must be saved to/loaded from the **I2C EEPROM (0x50)**. Failure to do so will reset sensor calibration on every reboot.

### 3. BMV080 Dust Sensor (Rule 6)
- **Life-Cycle:** The laser has a limited lifespan. It must be `startMeasurement()` during `P_WARMUP` and `stopMeasurement()` after data push.
- **Clock Stretching:** I2C timeout is globally set to **200ms** to accommodate BMV080's heavy internal processing.

### 4. Diagnostics (Rule 5.2)
- **Logowanie:** Use `ATLAS_LOG(level, verbose, fmt, ...)` exclusively.
- **Signaling:** Visual feedback is handled asynchronosuly via FreeRTOS `ledTask`. Do not use blocking `delay()` for LED patterns.

## 🚀 Deployment Standard
- **OTA Target:** 10.100.200.18
- **Board:** ESP32-S3 DevKitC-1 (N16R8)
- **Framework:** Arduino / PlatformIO
