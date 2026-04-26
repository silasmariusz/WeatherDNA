# Stevenson Screen (MUX 0x72) Subsystem - Logic & Architecture Blueprint

## Objective
This document provides a comprehensive, multi-step system architecture and logic blueprint for an environmental sensing node (Stevenson Screen). The design assumes the microcontroller communicates **exclusively** with sensors connected through a single TCA9548A I2C Multiplexer at address `0x72`.

This guide is designed to be read, understood, and implemented by an AI agent in subsequent coding tasks.

---

## Step 1: Hardware Abstraction & I2C Topology (The 0x72 Array)
**Goal:** Establish the hardware map and multiplexer isolation routines.

1.  **I2C Multiplexer (TCA9548A):**
    *   **Address:** `0x72`
    *   **Function:** Isolates sensors with identical addresses and prevents bus capacitance overload.
2.  **Sensor Map (Stevenson Screen Module):**
    *   `CH0` -> **BME688 (0x77)**: AI Gas Sensor (BSEC framework).
    *   `CH1` -> **ZMOD4510 (0x33)**: NO2 & O3 Industrial Gas Sensor.
    *   `CH2` -> **SCD41 (0x62) & SGP41 (0x59)**: NDIR CO2 and VOC/NOx index sensors (shared channel).
    *   `CH3` -> **BMV080 (0x57)**: Particulate Matter (PM1.0, PM2.5, PM10) Laser Sensor.
    *   `CH5` -> **SHT45 (0x44)**: Precision Temperature & Humidity.
    *   `CH7` -> **BMP585 (0x46)**: Ultra-precision Barometric Pressure & Altitude.
3.  **Routing Logic:**
    *   Create a `tcaselect(uint8_t i)` function.
    *   Before communicating with any sensor, send `1 << i` to the `0x72` address to open the corresponding channel.

---

## Step 2: System Modes & Lifecycle State Machine
**Goal:** Manage power efficiency and measurement cadences.

The system operates on a state machine driven by a 60-second cycle.

1.  **Operating Modes:**
    *   `MODE_CONTINUOUS`: Always awake. Sensors polled continuously.
    *   `MODE_LIGHT_SLEEP`: 15s awake, 45s sleep. CPU sleeps, but power to sensors remains active.
    *   `MODE_DEEP_SLEEP`: Sleeps for 5+ minutes. Hardware is shut down to save maximum power.
2.  **Cycle Phases (`currentPhase`):**
    *   `PHASE_WAKEUP`: Initialize I2C, boot the MUX, and run `begin()` on all 0x72 sensors.
    *   `PHASE_WARMUP`: Wait for hardware to stabilize. BMV080 laser spins up, ZMOD4510 heater stabilizes. Takes ~12s (Light Sleep) or ~45s (Cold Boot).
    *   `PHASE_READ`: Read fast-polling sensors (BME688, BMV080, SGP41) every second.
    *   `PHASE_PUSH_API`: Triggered at the 0-second mark of every minute. Execute a full read of all sensors, process math, and push to the cloud.
    *   `PHASE_SLEEP_WAIT`: Trigger sleep commands to sensors (e.g., SCD41 `powerDown()`, BMV080 `setMode(0)`) and invoke ESP32 sleep.

---

## Step 3: Sensor Initialization & Reading Strategy
**Goal:** Implement safe, non-blocking reads for the Stevenson Screen sensors.

1.  **High-Frequency Polling (Every 1s during active phase):**
    *   **BME688 (CH0):** Must call `bsec.run()` continuously to maintain the AI baseline.
    *   **BMV080 (CH3):** Continuously pull PM mass concentration to clear the hardware FIFO buffer.
    *   **SGP41 (CH2):** Feed the Sensirion algorithm with raw ticks to calculate VOC/NOx indices.
2.  **Synchronous Reading (Minute Mark):**
    *   **SHT45 (CH5):** Read Temperature (°C) and Humidity (%). Do this first, as these values are needed for compensating other sensors.
    *   **BMP585 (CH7):** Read Absolute Pressure (hPa).
    *   **SCD41 (CH2):** Trigger a `measureSingleShot()` to get CO2 (ppm) without heating the enclosure.
    *   **ZMOD4510 (CH1):** Trigger measurement using the SHT45 Temp/Hum values as input parameters for the Renesas SDK compensation algorithm.

---

## Step 4: Meteorological & Air Quality Data Fusion
**Goal:** Calculate derived biometeorological variables using exclusively the 0x72 dataset.

1.  **Meteorology (Requires SHT45 & BMP585):**
    *   *Dew Point:* `T - ((100 - H) / 5)`
    *   *Sea Level Pressure (SLP):* Compensate BMP585 absolute pressure using SHT45 temperature and known station altitude (e.g., 290m).
    *   *Absolute Humidity (g/m³):* Calculated via Clausius-Clapeyron equation using SHT45.
    *   *Heat Index / Feels Like:* Calculated if T > 27°C and H > 40%.
2.  **Pressure Trends (Zambretti Forecast):**
    *   Keep a rolling 3-hour buffer of BMP585 pressure readings.
    *   Calculate `dP/dt` (Delta Pressure over 3 hours).
    *   Determine weather trend (Falling = Storm, Rising = Fair, Stable = Clear).
3.  **Air Quality & Gas Fusion:**
    *   *Smog Index:* Combine BMV080 PM2.5 data with SGP41 NOx index and SHT45 Humidity.
    *   *WHO AQI:* Calculate the percentage of the WHO 24h limit using BMV080 PM2.5 and PM10.
    *   *Gas Pattern Identification:* Use BME688 Gas Estimates (1-4) combined with SCD41 CO2 and SGP41 VOC to classify the environment (e.g., "Wood Smoke", "Vehicle Exhaust", "Clean Air").

---

## Step 5: Network, Telemetry & Cloud Push
**Goal:** Transmit the collected and computed data.

1.  **Payload Construction:**
    *   Build a flat JSON object containing all raw SHT45, BMP585, SCD41, BMV080, ZMOD4510, SGP41, and BME688 parameters.
    *   Append the derived mathematical variables (Dew Point, SLP, Smog Index, etc.).
2.  **Endpoints:**
    *   **MQTT (Home Assistant):** Publish the JSON to `airsense/state`. Ensure MQTT Auto-Discovery topics are generated for each specific 0x72 sensor variable.
    *   **Custom API:** Send a POST request with the JSON payload to the main metrics server.
    *   **Weather Underground / Awekas:** Map SHT45 Temp/Hum and BMP585 SLP to the required GET parameters (convert °C to °F, hPa to inHg).

---

## Step 6: Fault Tolerance & Auto-Healing
**Goal:** Maintain system reliability if a sensor on the 0x72 bus crashes.

1.  **I2C Watchdog:**
    *   If `Wire.endTransmission()` returns an error on any channel during the cycle, increment a fail counter.
    *   If multiple sensors drop off, execute an I2C Bus Reset (`Wire.end()`, wait 100ms, `Wire.begin()`).
2.  **Glitch Guard:**
    *   If SHT45 returns Temp < -50°C or > 85°C, discard the reading to prevent corrupting the Sea Level Pressure math and the ZMOD4510 compensation logic.
    *   If BMV080 times out >15 times in the fast-poll loop, mark it offline dynamically and exclude it from the JSON payload to prevent `null` parsing errors on the server.

---

**End of Blueprint.**
*An AI implementing this system should initialize the TCA9548A first, instantiate the 0x72 sensor array, set up the 60-second phase loop, apply the biometeo formulas to the SHT45/BMP585 data, and bundle the output for MQTT.*
