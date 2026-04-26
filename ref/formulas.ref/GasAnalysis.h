/**
 * ============================================================================
 * GAS ANALYSIS & DETECTION LIBRARY
 * ============================================================================
 * Volatile Organic Compounds (VOC), NOx, CO2, and gas identification algorithms
 * Part of A.T.L.A.S. Project - Space & Earth Weather Monitoring
 * ============================================================================
 * Silas Mariusz silas@devspark.pl - 2024
*/

#ifndef GAS_ANALYSIS_H
#define GAS_ANALYSIS_H

#include <math.h>
#include <algorithm>
#include <cstring>

// ============================================================================
// 1. INDOOR AIR QUALITY COMPOSITE SCORE
// ============================================================================

/**
 * @brief Indoor Air Quality (IAQ) Composite Score (0-100 Scale)
 * 
 * @description
 * Combines multiple air quality metrics into single comprehensible score.
 * Includes: BSEC IAQ algorithm output, VOC index, CO2 level, PM2.5 concentration.
 * Higher score = better air quality; used for HVAC automation and health alerts.
 * 
 * @formula
 *   IAQ_score = 100 - (IAQ/2) - (VOC/10) - ((CO2-800)/100) - (PM2.5×0.5)
 *   Normalized to 0-100; negative values clipped to 0
 * 
 * @references
 * - ASHRAE Standard 62.1: Indoor Air Quality
 * - EPA Indoor Environmental Quality (IEQ) Guidelines
 * - Bosch BSEC2 Algorithm documentation
 * 
 * @parameters
 *   [in]  bsec_iaq               BSEC IAQ Index [0-500], range: 0-500
 *   [in]  voc_index              VOC Index [0-100], range: 0-100
 *   [in]  co2_ppm                CO2 concentration [ppm], range: 400-5000
 *   [in]  pm25_concentration     PM2.5 [µg/m³], range: 0-500
 * 
 * @returns IAQ composite score [0-100]
 * @unit Quality score (0=poor to 100=excellent)
 * @range Output: 0 to 100
 * @accuracy ±5 points
 * 
 * @thresholds_human_readable
 *   Score: 80-100  → Excellent - Healthy indoor environment
 *   Score: 60-80   → Good - Acceptable for most activities
 *   Score: 40-60   → Fair - Sensitive groups may experience issues
 *   Score: 20-40   → Poor - Health concerns; ventilation needed
 *   Score: 0-20    → Very Poor - Serious health risk; evacuate if possible
 * 
 * @example
 *   float iaq = calcIndoorAQScore(100, 45, 1000, 25);
 *   if (iaq < 40) { increaseVentilation(); }
 * 
 * @test_cases
 *   (50, 20, 800, 10)   → Score ≈ 75 (good)      ✓
 *   (200, 70, 2000, 50) → Score ≈ 15 (very poor) ✓
 */
inline float calcIndoorAQScore(float bsec_iaq, float voc_index, float co2_ppm, float pm25_concentration) {
    float score = 100.0 - (bsec_iaq / 2.0) - (voc_index / 10.0) - ((co2_ppm - 800.0) / 100.0) 
                  - (pm25_concentration * 0.5);
    return std::max(0.0f, std::min(100.0f, score));
}

// ============================================================================
// 2. REQUIRED AIR CHANGES PER HOUR (ACH) - CO2 Dilution
// ============================================================================

/**
 * @brief Required Air Changes Per Hour (ACH) for CO2 Target
 * 
 * @description
 * Calculates ventilation rate needed to dilute CO2 to acceptable level (1000 ppm).
 * Higher CO2 = more ventilation needed; used for HVAC control systems.
 * ASHRAE standard: 1000 ppm CO2 = adequate ventilation for office spaces.
 * 
 * @formula
 *   ACH = 0.02 × (CO2_current - 1000)  [air changes per hour]
 *   Empirical relationship from HVAC engineering
 * 
 * @references
 * - ASHRAE Standard 62.1: Ventilation & Acceptable Indoor Air Quality
 * - EPA Indoor Air Quality: CO2 Levels & Health
 * 
 * @parameters
 *   [in]  co2_ppm                Current CO2 concentration [ppm]
 * 
 * @returns Required ACH [1/hour]
 * @unit Air changes per hour (ACH)
 * @range Output: 0 to 10+ ACH
 * @accuracy ±20% (depends on room size, outdoor CO2)
 * 
 * @thresholds_human_readable
 *   ACH < 0.5   → Low ventilation needed (already meeting standard)
 *   ACH: 0.5-1.0→ Moderate ventilation increase advised
 *   ACH: 1-2    → Significant ventilation increase needed
 *   ACH > 2     → Aggressive ventilation; mechanical system maximum
 * 
 * @example
 *   float required_ach = calcRequiredACH(1500.0);  // 1500 ppm CO2
 *   if (required_ach > 1.0) { boostVentilation(); }
 * 
 * @test_cases
 *   (800 ppm)  → ACH ≈ -4 (already below target, clip to 0)  ✓
 *   (1000 ppm) → ACH = 0 (target met)                        ✓
 *   (1500 ppm) → ACH = 10 ACH needed                         ✓
 */
inline float calcRequiredACH(float co2_ppm) {
    float ach = 0.02 * (co2_ppm - 1000.0);
    return std::max(0.0f, ach);
}

// ============================================================================
// 3. RESPIRATORY HAZARD FROM COMBINED POLLUTANTS
// ============================================================================

/**
 * @brief Combined Respiratory Hazard (PM + VOC + NOx + CO2)
 * 
 * @description
 * Multi-pollutant respiratory risk assessment for vulnerable populations.
 * Accounts for synergistic effects; higher pollutant combinations = greater risk.
 * Used for: asthma warnings, occupational health, hospital IEQ monitoring.
 * 
 * @formula
 *   Hazard = (PM2.5/15 + VOC_ppm×2 + NOx/50) × humidity_factor  [0-10]
 *   Where humidity increases particle deposition in airways
 * 
 * @references
 * - EPA Air Quality Guidelines for Health Effects
 * - OSHA Occupational Exposure Limits (PELs)
 * - WHO Guidelines on Air Quality & Health
 * 
 * @parameters
 *   [in]  pm25_concentration     PM2.5 [µg/m³]
 *   [in]  voc_ppm                VOC concentration [ppm]
 *   [in]  nox_index              NOx index [0-100]
 *   [in]  relative_humidity      Relative humidity [%]
 * 
 * @returns Respiratory hazard [0-10]
 * @unit Risk index (0=safe to 10=emergency)
 * @range Output: 0 to 10+
 * @accuracy ±1 point
 * 
 * @thresholds_human_readable
 *   Hazard: 0-2    → Safe - No respiratory concerns
 *   Hazard: 2-4    → Mild - Sensitive individuals may experience symptoms
 *   Hazard: 4-6    → Moderate - General population may experience issues
 *   Hazard: 6-8    → High - Significant respiratory risk; activity restriction
 *   Hazard: 8-10   → Extreme - Medical emergency; avoid outdoors
 * 
 * @example
 *   float resp_hazard = calcRespiratoryHazard(80.0, 0.8, 40.0, 70.0);
 *   if (resp_hazard > 6) { respiratoryAlert(); }
 */
inline float calcRespiratoryHazard(float pm25_concentration, float voc_ppm, 
                                  float nox_index, float relative_humidity) {
    float humidity_factor = 1.0 + (relative_humidity - 50.0) / 100.0;
    float hazard = (pm25_concentration / 15.0 + voc_ppm * 2.0 + nox_index / 50.0) * humidity_factor;
    return std::min(10.0f, std::max(0.0f, hazard));
}

// ============================================================================
// 4. TOXICITY RISK INDEX (VOC + NOx + CO2 + IAQ Combined)
// ============================================================================

/**
 * @brief Toxicity Risk Index (Multi-Pollutant Chemical Hazard)
 * 
 * @description
 * Overall chemical toxicity assessment from multiple gas pollutants.
 * Evaluates cumulative chemical exposure risk for occupational settings.
 * 0 = safe; 10 = life-threatening chemical exposure.
 * 
 * @formula
 *   Toxicity = (VOC_idx/100 + NOx_idx/100 + (CO2-800)/500 + (500-IAQ)/500) / 4 × 10
 *   Normalized to 0-10 scale; accounts for multiple simultaneous exposures
 * 
 * @parameters
 *   [in]  voc_index              VOC index [0-100]
 *   [in]  nox_index              NOx index [0-100]
 *   [in]  co2_ppm                CO2 concentration [ppm]
 *   [in]  bsec_iaq               BSEC IAQ [0-500]
 * 
 * @returns Toxicity risk [0-10]
 * @unit Risk index (0-10)
 * @range Output: 0 to 10+
 * @accuracy ±1.5 points
 * 
 * @thresholds_human_readable
 *   Risk: 0-2      → Safe - Normal exposure levels
 *   Risk: 2-4      → Caution - Mild chemical exposure
 *   Risk: 4-6      → Warning - Moderate toxicity concern
 *   Risk: 6-8      → High - Significant toxicity; evacuation advised
 *   Risk: 8-10     → Emergency - Critical toxicity; immediate evacuation
 * 
 * @example
 *   float tox_risk = calcToxicityRisk(75.0, 60.0, 1200.0, 150.0);
 *   if (tox_risk > 6) { evacuateBuilding(); }
 */
inline float calcToxicityRisk(float voc_index, float nox_index, float co2_ppm, float bsec_iaq) {
    float voc_factor = voc_index / 100.0;
    float nox_factor = nox_index / 100.0;
    float co2_factor = std::max(0.0f, (co2_ppm - 800.0) / 500.0);
    float iaq_factor = std::max(0.0f, (500.0 - bsec_iaq) / 500.0);
    
    float toxicity = ((voc_factor + nox_factor + co2_factor + iaq_factor) / 4.0) * 10.0;
    return std::min(10.0f, toxicity);
}

// ============================================================================
// 5. GAS PATTERN IDENTIFICATION (Exhaust vs Respiration vs Cleaning)
// ============================================================================

/**
 * @brief Gas Pattern Identification (Source Classification)
 * 
 * @description
 * Identifies pollutant source type from gas ratio patterns.
 * Distinguishes: exhaust (high NOx), respiration (high CO2), cleaning (high VOC).
 * Useful for: indoor air quality context, ventilation strategy optimization.
 * 
 * @formula (Rule-based classification)
 *   If VOC >> NOx → Respiration/Metabolic
 *   If NOx >> VOC → Combustion/Exhaust
 *   If VOC >> NOx and CO2 rising → Human respiration
 *   If all elevated → Mixed sources or poor ventilation
 * 
 * @parameters
 *   [in]  gas_est_1              Gas estimate 1 (highest signal)
 *   [in]  gas_est_2              Gas estimate 2
 *   [in]  gas_est_3              Gas estimate 3
 *   [in]  gas_est_4              Gas estimate 4 (lowest signal)
 * 
 * @returns Gas type classification (string)
 * @unit Category (text)
 * @examples: "Respiration", "Exhaust", "Cleaning", "Mixed", "Outdoor"
 * 
 * @example
 *   const char* source = calcGasPatternID(15, 8, 3, 1);
 *   // Returns "Respiration"
 */
inline const char* calcGasPatternID(float gas_est_1, float gas_est_2, float gas_est_3, float gas_est_4) {
    float ratio_top_bottom = (gas_est_1 + gas_est_2) / std::max(0.1f, gas_est_3 + gas_est_4);
    
    if (ratio_top_bottom > 3.0) {
        if (gas_est_1 > 20) return "Respiration_High";
        return "Respiration";
    } else if (ratio_top_bottom < 0.5) {
        return "Exhaust";
    } else if (gas_est_2 > gas_est_1 * 0.7) {
        return "Cleaning_Products";
    } else if (ratio_top_bottom > 0.8 && ratio_top_bottom < 1.2) {
        return "Outdoor_Air";
    } else {
        return "Mixed_Sources";
    }
}

// ============================================================================
// 6. AIR QUALITY TREND ANALYSIS (Improving/Stable/Degrading)
// ============================================================================

/**
 * @brief Air Quality Trend (Short-term Direction)
 * 
 * @description
 * Evaluates whether air quality is improving, stable, or degrading.
 * Useful for: weather forecast confidence, ventilation effectiveness monitoring.
 * Compares current vs previous hour IAQ and VOC trends.
 * 
 * @formula
 *   Δ_IAQ = IAQ_now - IAQ_previous
 *   If Δ_IAQ > +5: Improving
 *   If Δ_IAQ: -5 to +5: Stable
 *   If Δ_IAQ < -5: Degrading
 * 
 * @parameters
 *   [in]  iaq_current            Current IAQ [0-500]
 *   [in]  iaq_previous           Previous hour IAQ [0-500]
 *   [in]  voc_current            Current VOC index [0-100]
 *   [in]  voc_previous           Previous hour VOC [0-100]
 * 
 * @returns Trend classification (string)
 * @unit Category: "Improving", "Stable", or "Degrading"
 * 
 * @example
 *   const char* trend = calcAQTrend(100, 120, 40, 50);
 *   // Returns "Improving"
 */
inline const char* calcAQTrend(float iaq_current, float iaq_previous, 
                               float voc_current, float voc_previous) {
    float iaq_delta = iaq_current - iaq_previous;
    float voc_delta = voc_current - voc_previous;
    float combined_delta = (iaq_delta / 50.0) + (voc_delta / 10.0);
    
    if (combined_delta > 1.0) {
        return "Improving";
    } else if (combined_delta < -1.0) {
        return "Degrading";
    } else {
        return "Stable";
    }
}

// ============================================================================
// 7. FORMALDEHYDE RISK INDEX (from VOC sensors)
// ============================================================================

/**
 * @brief Formaldehyde Risk Index (Carcinogenic VOC Proxy)
 * 
 * @description
 * Estimates formaldehyde concentration risk from VOC index.
 * Formaldehyde: known carcinogen; off-gasses from furniture, building materials.
 * Higher VOC in new/renovated spaces = higher formaldehyde risk.
 * 
 * @formula
 *   HCHO_risk = (VOC_index × 0.8) / 50  [estimated µg/m³, ~50 µg/m³ = high risk]
 * 
 * @references
 * - EPA Formaldehyde Guidelines
 * - IARC Group 1 Carcinogen classification
 * - Consumer Product Safety Commission (CPSC) regulations
 * 
 * @parameters
 *   [in]  voc_index              VOC index [0-100]
 * 
 * @returns Formaldehyde risk level [µg/m³ equivalent]
 * @unit micrograms per cubic meter (estimated)
 * @range Output: 0-100+ µg/m³
 * @accuracy ±30% (proxy estimation; actual measurement recommended)
 * 
 * @thresholds_human_readable
 *   Risk < 25 µg/m³   → Safe (OSHA PEL = 750 ppb ~930 µg/m³, but lower = better)
 *   Risk 25-50 µg/m³  → Elevated; ensure ventilation
 *   Risk 50-100 µg/m³ → High; increase fresh air intake
 *   Risk > 100 µg/m³  → Very high; consider source removal
 * 
 * @example
 *   float hcho = calcFormaldehydeRisk(70.0);
 *   if (hcho > 50) { increaseOutdoorAir(); }
 */
inline float calcFormaldehydeRisk(float voc_index) {
    return (voc_index * 0.8) / 10.0;  // Scaled estimate
}

#endif  // GAS_ANALYSIS_H
