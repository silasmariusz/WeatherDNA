# AirSense2 Project for AI Coding Agents

This project is an Arduino-based air quality monitoring system using an ESP32S3 microcontroller, managed with PlatformIO. It integrates various sensors for environmental data collection and employs different power management modes to optimize battery life.

## How to build

This project uses PlatformIO. To build the project, run the following command in the terminal:
```bash
platformio run -e seeed_xiao_esp32s3
```

## Important Considerations

The `src/main.cpp` file contains critical notes regarding the core architecture, power management, and sensor classifications. It is crucial to review these notes before making any modifications to the code.

## Project Structure

- `src/main.cpp`: Main application logic, sensor integration, and power management.
- `platformio.ini`: PlatformIO project configuration and dependency management.
- `docs/`: Contains detailed documentation for the project, including system architecture and data interpretation.
- `partitions.csv`: Custom partition table for the ESP32S3.

## Key Files and Directories

- `src/main.cpp`: The heart of the application.
- `platformio.ini`: Build configuration and library dependencies.
- `docs/EN_System_Architecture.md`: System architecture documentation.

## Libraries

The project uses several custom and external libraries. Custom libraries are located in `d:/Arduino/libraries.def/bsec_v3-3-0-0` and `d:/Arduino/libraries`.

## Versioning

This project uses conventional commits for easy versioning. Use commit messages like:
- `feat: add new feature` for new features (minor version bump)
- `fix: resolve bug` for bug fixes (patch version bump)
- `BREAKING CHANGE: description` for breaking changes (major version bump)

The release-please GitHub Action automatically creates releases and tags based on these commits.

## AI Agent / Skill Usage

Use the AirSense2 Copilot skill when editing firmware, troubleshooting I2C routing, or working with BME690/BSEC, BMV080, ZMOD4510, and other multiplexed sensors.

- Skill path: `.github/skills/airsense2/SKILL.md`
- Trigger via Copilot Chat by asking for AirSense2 firmware guidance, I2C mux handling, sensor integration, or debugging support.
