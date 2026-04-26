# Amateur Weather Networks & Data Publication

This document outlines the paths for contributing data from high-quality amateur weather stations (like WeatherDNA/AirScience) to national and global meteorological networks.

## 1. Global & National Networks

### A. CWOP (Citizen Weather Observer Program) - NOAA/NWS (USA/Global)
CWOP is the primary vehicle for private citizens to share data with the National Weather Service (NWS) and NOAA.
*   **Sign-up**: Register at [wxqa.com](http://www.wxqa.com/) to receive a CWOP Station ID (e.g., DW1234).
*   **Data Ingestion**: Data is fed into MADIS (Meteorological Assimilation Data Ingest System).
*   **Requirement**: Data must follow the APRS protocol or be bridged via software like WeeWX.

### B. IMGW-PIB (Poland)
IMGW does not currently offer a direct automated API for private station ingestion into official models, but there are indirect routes:
*   **Skywarn Polska (Polscy £owcy Burz)**: Participating in reports of severe phenomena.
*   **Platforma Obserwator**: Manual reporting of current conditions.
*   **Global Mesonets**: IMGW researchers and synopticians often monitor broader networks like **Weather Underground** and **Awekas** for regional high-resolution studies.

## 2. Recommended Amateur Networks
Sharing data with these services ensures your high-quality readings are used by local communities and climate researchers:

1.  **Weather Underground (WU)**: The largest global personal weather station network.
2.  **Weathercloud**: Modern, high-resolution dashboard with social features.
3.  **Awekas**: Austrian-based network popular in Europe, known for strict quality control.
4.  **PWS Weather**: Part of the AerisWeather network.
5.  **Windy.com**: Allows PWS uploads to show high-resolution local maps.

## 3. Data Quality Standards
To ensure data is accepted and useful:
*   **Barometric Pressure**: Must be reported as **Sea Level Pressure (SLP)**.
*   **Siting**: Stevenson Screen for temperature/humidity (approx. 2m above grass).
*   **Calibration**: Regularly verify readings against reference instruments.
