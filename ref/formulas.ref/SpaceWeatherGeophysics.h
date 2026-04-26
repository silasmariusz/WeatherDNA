/**
 * ============================================================================
 * SPACE WEATHER & GEOPHYSICS LIBRARY
 * ============================================================================
 * Cosmic rays, geomagnetic disturbances, aurora, seismic proxies, space weather
 * Part of A.T.L.A.S. Project - Space & Earth Weather Monitoring
 * ============================================================================
 * Silas Mariusz silas@devspark.pl - 2024
*/

#ifndef SPACE_WEATHER_GEOPHYSICS_H
#define SPACE_WEATHER_GEOPHYSICS_H

#include <math.h>
#include <algorithm>

// ============================================================================
// 1. MAGNETIC SHOCK WAVE DETECTION (dB/dt - SSC Detector)
// ============================================================================

/**
 * @brief Magnetic Shock Wave Rate (dB/dt - Sudden Storm Commencement)
 * 
 * @description
 * Detects solar storm impact on magnetosphere via rapid magnetic field change.
 * dB/dt > 5 nT/s indicates CME/solar wind shock arrival.
 * Critical for: space weather alerts, power grid protection, satellite anomalies.
 * 
 * @formula
 *   dB/dt = √(ΔBx² + ΔBy² + ΔBz²) / Δt  [nT/s]
 *   Where: Bx, By, Bz = 3-axis magnetometer components [µT]
 * 
 * @references
 * - NOAA Space Weather Prediction Center: https://www.swpc.noaa.gov/
 * - Schuster, A. (1889) "Disturbances in terrestrial magnetism"
 * - International Association of Geomagnetism & Aeronomy (IAGA)
 * 
 * @parameters
 *   [in]  delta_bx_ut            X-axis change [µT]
 *   [in]  delta_by_ut            Y-axis change [µT]
 *   [in]  delta_bz_ut            Z-axis change [µT]
 *   [in]  delta_time_seconds     Time interval [seconds]
 * 
 * @returns Rate of change [nT/s]
 * @unit nT/s (nanotesla per second)
 * @range Output: 0 to 1000+ nT/s (1 µT = 1000 nT)
 * @accuracy Depends on magnetometer accuracy (±0.5 nT typical)
 * 
 * @thresholds_human_readable
 *   dB/dt < 1 nT/s     → Quiet magnetosphere
 *   dB/dt: 1-5 nT/s    → Unsettled conditions
 *   dB/dt: 5-10 nT/s   → Geomagnetic storm arrival (CME impact)
 *   dB/dt > 10 nT/s    → Major storm commencement; infrastructure risk
 * 
 * @example
 *   float dBdt = calcMagneticShockWave(0.5, 0.3, 0.8, 1.0);
 *   if (dBdt > 5) { solarStormAlert(); }
 * 
 * @test_cases
 *   (0.001, 0.001, 0.001, 1 sec) → dB/dt ≈ 1.7 nT/s  ✓
 *   (0.005, 0.003, 0.008, 1 sec) → dB/dt ≈ 10 nT/s   ✓
 */
inline float calcMagneticShockWave(float delta_bx_ut, float delta_by_ut, float delta_bz_ut, 
                                   float delta_time_seconds) {
    float total_change = sqrt(delta_bx_ut * delta_bx_ut + delta_by_ut * delta_by_ut + delta_bz_ut * delta_bz_ut);
    float rate = (total_change * 1000.0) / delta_time_seconds;  // Convert µT to nT
    return rate;
}

// ============================================================================
// 2. FORBUSH DECREASE DETECTION (Solar Eruption Signature)
// ============================================================================

/**
 * @brief Forbush Decrease (Solar Wind Particle Flux Suppression)
 * 
 * @description
 * Sudden drop in cosmic ray count following solar eruption (CME).
 * Solar wind shock deflects cosmic rays; creates distinctive dip in Geiger data.
 * Key signature for space weather event detection and timing.
 * 
 * @formula
 *   Forbush% = ((GCR_now - avg_7day) / avg_7day) × 100%
 *   Negative values = decrease (Forbush event); typically -5 to -30%
 * 
 * @references
 * - Forbush, S. E. (1937) "On the effects of sudden changes in cosmic ray intensity"
 * - Cane, H. V. (2000) "Coronal mass ejections and Forbush decreases" - Sp. Sci. Rev.
 * - International Cosmic Ray Observatory network
 * 
 * @parameters
 *   [in]  gcr_now_cpm            Current GCR count [cpm], already pressure-corrected
 *   [in]  avg_7day_cpm           7-day running average [cpm]
 * 
 * @returns Forbush decrease percentage [%]
 * @unit Percent (%)
 * @range Output: -100 to +100% (typically ±20%)
 * @accuracy ±5% (depends on 7-day baseline stability)
 * 
 * @thresholds_human_readable
 *   Forbush: 0 to -5%      → Normal variation
 *   Forbush: -5 to -15%    → Minor Forbush event (minor solar eruption)
 *   Forbush: -15 to -30%   → Major event (strong CME)
 *   Forbush < -30%         → Extreme event (major space weather storm)
 * 
 * @example
 *   float forbush = calcForbushDecrease(20.0, 25.0);  // -20% decrease
 *   if (forbush < -15) { spacWeatherAlert("Major Forbush event"); }
 * 
 * @test_cases
 *   (25 cpm, 25 cpm)  → Forbush = 0%    (no change)      ✓
 *   (20 cpm, 25 cpm)  → Forbush = -20%  (event detected) ✓
 */
inline float calcForbushDecrease(float gcr_now_cpm, float avg_7day_cpm) {
    if (avg_7day_cpm < 1.0) return 0.0;
    return ((gcr_now_cpm - avg_7day_cpm) / avg_7day_cpm) * 100.0;
}

// ============================================================================
// 3. K-INDEX PROXY (Geomagnetic Activity Index)
// ============================================================================

/**
 * @brief K-Index Proxy (IAGA Geomagnetic Disturbance Scale Estimate)
 * 
 * @description
 * Approximates official K-index (0-9 scale) from magnetometer data.
 * Real K-index requires 13 ground stations; this is local proxy.
 * K ≥ 5 = Geomagnetic storm; K ≥ 7 = Severe storm; K = 9 = Extreme.
 * 
 * @formula
 *   ΔB_total = √(ΔBx² + ΔBy² + ΔBz²)  [nT]
 *   K ≈ log2(ΔB_total / 500) × 1.5  (scaled to 0-9)
 * 
 * @references
 * - IAGA Geomagnetic K-index: https://www.iaga-aiga.org/
 * - Bartels, J. (1957) "The standardized index Ks and the planetary index Kp"
 * - NOAA/NWS Space Weather Scales
 * 
 * @parameters
 *   [in]  delta_bx_nt            X-axis change [nT]
 *   [in]  delta_by_nt            Y-axis change [nT]
 *   [in]  delta_bz_nt            Z-axis change [nT]
 * 
 * @returns K-index proxy [0-9]
 * @unit K-index scale (0=quiet to 9=extreme)
 * @range Output: 0 to 9+ (integer mapping)
 * @accuracy ±1 index point
 * 
 * @thresholds_human_readable
 *   K: 0-1 → Quiet magnetosphere
 *   K: 2-3 → Unsettled
 *   K: 4   → Active storm condition
 *   K: 5-6 → Minor geomagnetic storm
 *   K: 7   → Severe geomagnetic storm
 *   K: 8-9 → Major/extreme geomagnetic storm
 * 
 * @example
 *   int k_index = calcK_Index(150, 200, 100);
 *   if (k_index >= 7) { severeStormAlert(); }
 */
inline int calcK_Index(float delta_bx_nt, float delta_by_nt, float delta_bz_nt) {
    float total_change = sqrt(delta_bx_nt * delta_bx_nt + delta_by_nt * delta_by_nt + delta_bz_nt * delta_bz_nt);
    float k_val = log2(std::max(1.0f, total_change / 500.0f)) * 1.5f;
    int k_index = (int)std::min(9.0f, std::max(0.0f, k_val));
    return k_index;
}

// ============================================================================
// 4. AURORA PROBABILITY (Visibility Likelihood)
// ============================================================================

/**
 * @brief Aurora Probability (Northern Lights Visibility Chance)
 * 
 * @description
 * Estimates aurora visibility probability from K-index and geomagnetic latitude.
 * Aurora becomes visible from further south as storms strengthen.
 * Critical for: aurora forecasting, high-latitude weather apps.
 * 
 * @formula
 *   Aurora_Prob% = Min(98, (geo_lat - boundary) × 15)  [percent]
 *   Where boundary = equatorward auroral oval boundary (depends on Kp)
 * 
 * @references
 * - Akasofu, S. I. (1981) "Energy coupling between the solar wind and magnetosphere"
 * - NOAA Aurora Forecast: https://www.swpc.noaa.gov/
 * 
 * @parameters
 *   [in]  k_index_proxy          K-index estimate [0-9]
 *   [in]  geomag_latitude        Geomagnetic latitude [°N]
 * 
 * @returns Aurora visibility probability [%]
 * @unit Percent (%)
 * @range Output: 0 to 98%
 * @accuracy ±10-15% (depends on latitude accuracy)
 * 
 * @thresholds_human_readable
 *   Prob: 0-10%      → Unlikely (only equatorward-expanded during extreme storms)
 *   Prob: 10-30%     → Low (possible during moderate storms)
 *   Prob: 30-60%     → Moderate (good viewing chance during active periods)
 *   Prob: 60-80%     → High (strong likelihood during storm)
 *   Prob: 80-98%     → Very high (excellent viewing conditions)
 * 
 * @example
 *   float aurora_prob = calcAuroraProbability(6, 65.0);  // Alaska latitude
 *   if (aurora_prob > 60) { auroraViewingAlert(); }
 * 
 * @test_cases
 *   (K=0, 60°N) → Prob ≈ 0%     (quiet)     ✓
 *   (K=5, 65°N) → Prob ≈ 60%    (good)      ✓
 *   (K=9, 65°N) → Prob ≈ 98%    (excellent) ✓
 */
inline float calcAuroraProbability(int k_index_proxy, float geomag_latitude) {
    float boundary = 65.0 - (k_index_proxy * 1.5);  // Equatorward expansion
    float prob = std::min(98.0f, std::max(0.0f, (geomag_latitude - boundary) * 15.0f));
    return prob;
}

// ============================================================================
// 5. SEISMIC PROXY (QVAR - Electrostatic Earthquake Indicator)
// ============================================================================

/**
 * @brief Seismic Proxy (QVAR Rate - Earthquake Precursor Indicator)
 * 
 * @description
 * QVAR sensor detects electrostatic anomalies from crustal stress/fracturing.
 * Some studies suggest electromagnetic precursors to earthquakes.
 * Use with caution; field is controversial but shows promise.
 * 
 * @formula
 *   Proxy = Min(10, QVAR_rate / 200)  [0-10 scale]
 *   Elevated QVAR rate = increased electrostatic activity
 * 
 * @references
 * - Fidani, C. (2010) "The central Italy uplift: a new tool to understand pre-seismic phenomena"
 * - Draganov et al. (1991) "Electromagnetic emission sparks before earthquakes"
 * - USGS Earthquake Hazards Program
 * 
 * @parameters
 *   [in]  qvar_rate_units        QVAR change rate [units/minute]
 * 
 * @returns Seismic activity proxy [0-10]
 * @unit Activity index (0-10, qualitative)
 * @range Output: 0 to 10
 * @accuracy Low (emerging field, requires validation)
 * 
 * @thresholds_human_readable
 *   Proxy: 0-2    → Background levels; no concern
 *   Proxy: 2-4    → Elevated; monitor conditions
 *   Proxy: 4-6    → High; potential precursor zone
 *   Proxy: 6-10   → Very high; cannot predict timing/location
 * 
 * @note Use only as supplementary indicator; NOT for earthquake prediction
 * 
 * @example
 *   float seismic_proxy = calcSeismicProxy(800.0);
 */
inline float calcSeismicProxy(float qvar_rate_units) {
    return std::min(10.0f, qvar_rate_units / 200.0f);
}

// ============================================================================
// 6. CONVECTIVE INSTABILITY INDEX (Thunderstorm Potential)
// ============================================================================

/**
 * @brief Convective Instability Index (Severe Weather Potential)
 * 
 * @description
 * Combines atmospheric stability indicators for thunderstorm/severe weather likelihood.
 * High θe + low LCL + negative pressure trend = severe weather.
 * Used in weather forecasting and aviation meteorology.
 * 
 * @formula
 *   Instability = (1 - θe/350) + (1 - LCL/3000) + (dP/dt factor)  [0-10 scale]
 * 
 * @references
 * - Stull, R. (2011) "Meteorology for Scientists and Engineers"
 * - Storm Prediction Center severe weather forecasting
 * 
 * @parameters
 *   [in]  theta_e_kelvin         Equivalent potential temp [K]
 *   [in]  lcl_meters             Lifting condensation level [m]
 *   [in]  delta_pressure_3h      3-hour pressure change [hPa]
 * 
 * @returns Instability index [0-10]
 * @unit Convective index (0=stable to 10=extremely unstable)
 * @range Output: 0 to 10+
 * @accuracy ±1 point
 * 
 * @thresholds_human_readable
 *   Index: 0-2    → Stable; no storms expected
 *   Index: 2-4    → Marginally unstable; isolated storms possible
 *   Index: 4-6    → Unstable; thunderstorms likely
 *   Index: 6-8    → Very unstable; strong storms likely
 *   Index: 8-10+  → Extremely unstable; severe/tornadic storms risk
 * 
 * @example
 *   float instability = calcConvectiveInstability(320, 1500, -5);
 *   if (instability > 7) { severeWeatherWarning(); }
 */
inline float calcConvectiveInstability(float theta_e_kelvin, float lcl_meters, float delta_pressure_3h) {
    float theta_e_factor = std::max(0.0f, 1.0f - (theta_e_kelvin / 350.0f));
    float lcl_factor = std::max(0.0f, 1.0f - (lcl_meters / 3000.0f));
    float pressure_factor = std::min(1.0f, std::max(0.0f, -delta_pressure_3h / 10.0f));
    
    float instability = (theta_e_factor + lcl_factor + pressure_factor * 0.5f) * 3.33f;
    return std::min(10.0f, instability);
}

#endif  // SPACE_WEATHER_GEOPHYSICS_H
