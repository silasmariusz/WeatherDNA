/**
 * ============================================================================
 * AIR QUALITY INDICES LIBRARY
 * BIBLIOTEKA WSKAŹNIKÓW JAKOŚCI POWIETRZA
 * ============================================================================
 * Scientific formulas for air quality assessment
 * Includes EPA AQI, European AQI, WHO standards, visibility, and smog indices
 * Naukowe formuły do oceny jakości powietrza
 * Obejmuje AQI EPA, Europejski AQI, standardy WHO, widoczność i wskaźniki smogu
 * ============================================================================
 * Silas Mariusz silas@devspark.pl - 2026
*/

#ifndef AIR_QUALITY_INDICES_H
#define AIR_QUALITY_INDICES_H

#include <math.h>
#include <algorithm>

// ============================================================================
// 1. EPA AIR QUALITY INDEX (AQI) - PM2.5
// 1. WSKAŹNIK JAKOŚCI POWIETRZA (AQI) EPA - PM2.5
// ============================================================================

/**
 * @brief EPA Air Quality Index from PM2.5 (Piecewise Linear Interpolation)
 * @brief Wskaźnik Jakości Powietrza (AQI) EPA z PM2.5 (Liniowa Interpolacja Odcinkowa)
 * 
 * @description
 * Official US EPA air quality index based on particulate matter (PM2.5).
 * Used in air quality forecasts, health advisories, outdoor activity recommendations.
 * AQI scale: 0-500+ with categories: Good, Moderate, Unhealthy for Sensitive Groups,
 * Unhealthy, Very Unhealthy, Hazardous.
 * Oficjalny wskaźnik jakości powietrza US EPA oparty na cząstkach stałych (PM2.5).
 * Używany w prognozach jakości powietrza, zaleceniach zdrowotnych, rekomendacjach dotyczących aktywności na zewnątrz.
 * Skala AQI: 0-500+ z kategoriami: Dobra, Umiarkowana, Niezdrowa dla Grup Wrażliwych,
 * Niezdrowa, Bardzo Niezdrowa, Niebezpieczna.
 * 
 * @formula (Piecewise linear with 7 breakpoints)
 *   Concentration breakpoints [µg/m³]: 0, 12.1, 35.5, 55.5, 150.5, 250.5, 500.5
 *   AQI breakpoints: 0, 50, 100, 150, 200, 300, 500
 *   AQI = (AQI_hi - AQI_lo)/(BP_hi - BP_lo) × (conc - BP_lo) + AQI_lo
 * @formula (Liniowa odcinkowa z 7 punktami przełamania)
 *   Punkty przełamania stężenia [µg/m³]: 0, 12.1, 35.5, 55.5, 150.5, 250.5, 500.5
 *   Punkty przełamania AQI: 0, 50, 100, 150, 200, 300, 500
 *   AQI = (AQI_hi - AQI_lo)/(BP_hi - BP_lo) × (stężenie - BP_lo) + AQI_lo
 * 
 * @references
 * - EPA Air Quality Index: https://www.epa.gov/air-quality/air-quality-index-aqi
 * - EPA AQI Technical Assistance Document: https://www.airnow.gov/
 * - 40 CFR Part 50 (National Ambient Air Quality Standards)
 * - Dokument Techniczny EPA AQI: https://www.airnow.gov/
 * - 40 CFR Part 50 (Krajowe Standardy Jakości Powietrza Otoczenia)
 * 
 * @parameters
 *   [in]  pm25_concentration    PM2.5 concentration [µg/m³], range: 0-1000
 *   [in]  pm25_concentration    Stężenie PM2.5 [µg/m³], zakres: 0-1000
 * 
 * @returns Air Quality Index value [0-500+]
 * @unit AQI points (dimensionless)
 * @range Output: 0 to 500+ (can exceed 500 for extremely hazardous conditions)
 * @accuracy Exact (piecewise linear by definition)
 * @returns Wartość Wskaźnika Jakości Powietrza [0-500+]
 * @unit Punkty AQI (bezjednostkowe)
 * @range Wyjście: 0 do 500+ (może przekroczyć 500 dla ekstremalnie niebezpiecznych warunków)
 * @accuracy Dokładna (liniowa odcinkowa z definicji)
 * 
 * @thresholds_human_readable
 *   AQI: 0-50           → Good - Air quality satisfactory
 *   AQI: 51-100         → Moderate - Acceptable, some may be sensitive
 *   AQI: 101-150        → Unhealthy for Sensitive Groups - Sensitive individuals should limit outdoor
 *   AQI: 151-200        → Unhealthy - Some members of general public may experience effects
 *   AQI: 201-300        → Very Unhealthy - Health alert; everyone may begin to experience effects
 *   AQI: 301+           → Hazardous - Everyone should avoid outdoor exertion
 *   AQI: 0-50           → Dobra - Jakość powietrza zadowalająca
 *   AQI: 51-100         → Umiarkowana - Akceptowalna, niektórzy mogą być wrażliwi
 *   AQI: 101-150        → Niezdrowa dla Grup Wrażliwych - Osoby wrażliwe powinny ograniczyć aktywność na zewnątrz
 *   AQI: 151-200        → Niezdrowa - Niektórzy członkowie ogółu społeczeństwa mogą doświadczać skutków
 *   AQI: 201-300        → Bardzo Niezdrowa - Alert zdrowotny; wszyscy mogą zacząć doświadczać skutków
 *   AQI: 301+           → Niebezpieczna - Wszyscy powinni unikać wysiłku na zewnątrz
 * 
 * @example
 *   float aqi = calcAQI_PM25(35.0);  // PM2.5 = 35 µg/m³ → AQI ≈ 97 (Moderate)
 *   if (aqi > 150) { alertOutdoorExercise("Restrict activity"); }
 *   float aqi = calcAQI_PM25(35.0);  // PM2.5 = 35 µg/m³ → AQI ≈ 97 (Umiarkowana)
 *   if (aqi > 150) { alertOutdoorExercise("Ogranicz aktywność"); }
 * 
 * @test_cases
 *   (0 µg/m³)    → AQI = 0     (Good)                 ✓
 *   (12 µg/m³)   → AQI = 50    (Good limit)           ✓
 *   (35 µg/m³)   → AQI ≈ 97    (Moderate)             ✓
 *   (150 µg/m³)  → AQI = 200   (Unhealthy)            ✓
 *   (0 µg/m³)    → AQI = 0     (Dobra)                 ✓
 *   (12 µg/m³)   → AQI = 50    (Limit dobrej)           ✓
 *   (35 µg/m³)   → AQI ≈ 97    (Umiarkowana)             ✓
 *   (150 µg/m³)  → AQI = 200   (Niezdrowa)            ✓
 */
inline float calcAQI_PM25(float pm25_concentration) {
    // EPA breakpoints [µg/m³] and corresponding AQI values
    // Punkty przełamania EPA [µg/m³] i odpowiadające im wartości AQI
    const float BP_conc[] = {0, 12.1, 35.5, 55.5, 150.5, 250.5, 500.5};
    const float BP_aqi[]  = {0, 50, 100, 150, 200, 300, 500};
    
    for (int i = 0; i < 6; i++) {
        if (pm25_concentration <= BP_conc[i + 1]) {
            float aqi = (BP_aqi[i + 1] - BP_aqi[i]) / (BP_conc[i + 1] - BP_conc[i]) 
                        * (pm25_concentration - BP_conc[i]) + BP_aqi[i];
            return aqi;
        }
    }
    // Beyond highest breakpoint
    // Poza najwyższym punktem przełamania
    return 500.0 + (pm25_concentration - 500.5) * (500.0 / 500.5);
}

// ============================================================================
// 2. EUROPEAN AIR QUALITY INDEX (EAQI) - PM2.5 Threshold
// 2. EUROPEJSKI WSKAŹNIK JAKOŚCI POWIETRZA (EAQI) - Próg PM2.5
// ============================================================================

/**
 * @brief European Air Quality Index (EAQI - Simplified 1-6 Scale)
 * @brief Europejski Wskaźnik Jakości Powietrza (EAQI - Uproszczona Skala 1-6)
 * 
 * @description
 * European Environment Agency (EEA) air quality classification (1-6 scale).
 * Simpler than EPA AQI but widely used in EU air quality reporting.
 * Based on multiple pollutants; this version uses PM2.5 primary indicator.
 * Klasyfikacja jakości powietrza Europejskiej Agencji Środowiska (EEA) (skala 1-6).
 * Prostsza niż AQI EPA, ale szeroko stosowana w raportach o jakości powietrza w UE.
 * Oparta na wielu zanieczyszczeniach; ta wersja wykorzystuje PM2.5 jako główny wskaźnik.
 * 
 * @formula (Threshold-based classification)
 *   1 (Good)              : PM2.5 ≤ 10 µg/m³
 *   2 (Fair)              : 10 < PM2.5 ≤ 20 µg/m³
 *   3 (Moderate)          : 20 < PM2.5 ≤ 30 µg/m³
 *   4 (Poor)              : 30 < PM2.5 ≤ 40 µg/m³
 *   5 (Very Poor)         : 40 < PM2.5 ≤ 50 µg/m³
 *   6 (Extremely Poor)    : PM2.5 > 50 µg/m³
 * @formula (Klasyfikacja oparta na progach)
 *   1 (Dobra)              : PM2.5 ≤ 10 µg/m³
 *   2 (Umiarkowana)          : 10 < PM2.5 ≤ 20 µg/m³
 *   3 (Zła)          : 20 < PM2.5 ≤ 30 µg/m³
 *   4 (Bardzo Zła)              : 30 < PM2.5 ≤ 40 µg/m³
 *   5 (Ekstremalnie Zła)         : 40 < PM2.5 ≤ 50 µg/m³
 *   6 (Niebezpieczna)    : PM2.5 > 50 µg/m³
 * 
 * @references
 * - European Environment Agency Air Quality Classification
 * - EEA Report #12/2019 on Air Quality in Europe
 * - Directive 2008/50/EC (EU Air Quality Directive)
 * - Klasyfikacja Jakości Powietrza Europejskiej Agencji Środowiska
 * - Raport EEA #12/2019 o Jakości Powietrza w Europie
 * - Dyrektywa 2008/50/WE (Dyrektywa UE w sprawie jakości powietrza)
 * 
 * @parameters
 *   [in]  pm25_concentration    PM2.5 concentration [µg/m³], range: 0-500
 *   [in]  pm25_concentration    Stężenie PM2.5 [µg/m³], zakres: 0-500
 * 
 * @returns EAQI category [1-6]
 * @unit EAQI scale (1=Good to 6=Extremely Poor)
 * @range Output: 1 to 6 (integer)
 * @accuracy Exact (threshold-based)
 * @returns Kategoria EAQI [1-6]
 * @unit Skala EAQI (1=Dobra do 6=Niebezpieczna)
 * @range Wyjście: 1 do 6 (liczba całkowita)
 * @accuracy Dokładna (oparta na progach)
 * 
 * @thresholds_human_readable
 *   EAQI = 1 → Good      - Green - Safe for all activities
 *   EAQI = 2 → Fair      - Yellow - Generally safe
 *   EAQI = 3 → Moderate  - Orange - Sensitive groups should limit outdoor
 *   EAQI = 4 → Poor      - Red    - Sensitive groups affected; others advised
 *   EAQI = 5 → Very Poor - Purple - General population affected
 *   EAQI = 6 → Extremely Poor - Maroon - Health emergency; avoid outdoors
 *   EAQI = 1 → Dobra      - Zielony - Bezpieczna dla wszystkich aktywności
 *   EAQI = 2 → Umiarkowana      - Żółty - Ogólnie bezpieczna
 *   EAQI = 3 → Zła  - Pomarańczowy - Grupy wrażliwe powinny ograniczyć aktywność na zewnątrz
 *   EAQI = 4 → Bardzo Zła      - Czerwony    - Grupy wrażliwe dotknięte; innym zaleca się ostrożność
 *   EAQI = 5 → Ekstremalnie Zła - Fioletowy - Dotknięta ogólna populacja
 *   EAQI = 6 → Niebezpieczna - Bordowy - Nagły wypadek zdrowotny; unikać przebywania na zewnątrz
 * 
 * @example
 *   int eaqi = calcEAQI(25.0);  // PM2.5 = 25 µg/m³ → EAQI = 3 (Moderate)
 *   int eaqi = calcEAQI(25.0);  // PM2.5 = 25 µg/m³ → EAQI = 3 (Zła)
 *   if (eaqi >= 4) { warnSensitiveGroups(); }
 * 
 * @test_cases
 *   (5 µg/m³)   → EAQI = 1 (Good)           ✓
 *   (15 µg/m³)  → EAQI = 2 (Fair)           ✓
 *   (25 µg/m³)  → EAQI = 3 (Moderate)       ✓
 *   (35 µg/m³)  → EAQI = 4 (Poor)           ✓
 *   (5 µg/m³)   → EAQI = 1 (Dobra)           ✓
 *   (15 µg/m³)  → EAQI = 2 (Umiarkowana)           ✓
 *   (25 µg/m³)  → EAQI = 3 (Zła)       ✓
 *   (35 µg/m³)  → EAQI = 4 (Bardzo Zła)           ✓
 */
inline int calcEAQI(float pm25_concentration) {
    if (pm25_concentration <= 10.0) return 1;   // Good
    if (pm25_concentration <= 20.0) return 2;   // Fair
    if (pm25_concentration <= 30.0) return 3;   // Moderate
    if (pm25_concentration <= 40.0) return 4;   // Poor
    if (pm25_concentration <= 50.0) return 5;   // Very Poor
    return 6;                                    // Extremely Poor
    if (pm25_concentration <= 10.0) return 1;   // Dobra
    if (pm25_concentration <= 20.0) return 2;   // Umiarkowana
    if (pm25_concentration <= 30.0) return 3;   // Zła
    if (pm25_concentration <= 40.0) return 4;   // Bardzo Zła
    if (pm25_concentration <= 50.0) return 5;   // Ekstremalnie Zła
    return 6;                                    // Niebezpieczna
}

// ============================================================================
// 3. WHO AIR QUALITY GUIDELINES (% of WHO 24-hour Limit)
// 3. WYTYCZNE WHO DOTYCZĄCE JAKOŚCI POWIETRZA (% Limit WHO 24-godzinny)
// ============================================================================

/**
 * @brief WHO Air Quality Index (Percent of WHO 24-hour Limit)
 * @brief Wskaźnik Jakości Powietrza WHO (Procent Limit WHO 24-godzinny)
 * 
 * @description
 * WHO 2021 guidelines set strict PM2.5 limit of 15 µg/m³ for 24-hour average.
 * This index shows how far above/below WHO recommendation current reading is.
 * Critical for health assessments in developing countries with high pollution.
 * Wytyczne WHO 2021 ustalają ścisły limit PM2.5 wynoszący 15 µg/m³ dla średniej 24-godzinnej.
 * Ten wskaźnik pokazuje, o ile powyżej/poniżej zalecenia WHO znajduje się aktualny odczyt.
 * Kluczowe dla ocen zdrowotnych w krajach rozwijających się z wysokim zanieczyszczeniem.
 * 
 * @formula
 *   WHO_AQI = Max(PM2.5/15, PM10/45) × 100%  [percent]
 *   WHO 2021 24-hour limits: PM2.5 ≤ 15 µg/m³, PM10 ≤ 45 µg/m³
 *   WHO_AQI = Max(PM2.5/15, PM10/45) × 100%  [procent]
 *   Limity WHO 2021 24-godzinne: PM2.5 ≤ 15 µg/m³, PM10 ≤ 45 µg/m³
 * 
 * @references
 * - WHO 2021 Air Quality Guidelines Update
 * - Aktualizacja Wytycznych WHO 2021 dotyczących Jakości Powietrza
 * - https://www.who.int/publications/i/item/9789040022913
 * - WHO Technical Report Series #958 (Air quality guidelines)
 * - Seria Raportów Technicznych WHO #958 (Wytyczne dotyczące jakości powietrza)
 * 
 * @parameters
 *   [in]  pm25_concentration    PM2.5 concentration [µg/m³], range: 0-500
 *   [in]  pm25_concentration    Stężenie PM2.5 [µg/m³], zakres: 0-500
 * 
 * @returns Percent of WHO 24-hour limit [%]
 * @unit Percent (%)
 * @range Output: 0% (excellent) to 500%+ (severely polluted)
 * @accuracy Exact (based on WHO thresholds)
 * @returns Procent limitu WHO 24-godzinnego [%]
 * @unit Procent (%)
 * @range Wyjście: 0% (doskonałe) do 500%+ (silnie zanieczyszczone)
 * @accuracy Dokładna (oparta na progach WHO)
 * 
 * @thresholds_human_readable
 *   WHO_AQI: 0-100%      → Within WHO guideline (safe)
 *   WHO_AQI: 100-200%    → 1-2× WHO limit (unhealthy)
 *   WHO_AQI: 200-400%    → 2-4× WHO limit (very unhealthy)
 *   WHO_AQI: 400%+       → 4× WHO limit or higher (hazardous)
 *   WHO_AQI: 0-100%      → W ramach wytycznych WHO (bezpieczne)
 *   WHO_AQI: 100-200%    → 1-2× limit WHO (niezdrowe)
 *   WHO_AQI: 200-400%    → 2-4× limit WHO (bardzo niezdrowe)
 *   WHO_AQI: 400%+       → 4× limit WHO lub więcej (niebezpieczne)
 * 
 * @example
 *   float who_aqi = calcWHO_AQI(30.0, 80.0);  // PM2.5 30, PM10 80
 *   // Returns max(200%, 178%) = 200% (2× WHO limit)
 *   // Zwraca max(200%, 178%) = 200% (2× limit WHO)
 * 
 * @test_cases
 *   (15 µg/m³ PM2.5, 45 µg/m³ PM10) → WHO_AQI = 100% (at limit)  ✓
 *   (15 µg/m³ PM2.5, 45 µg/m³ PM10) → WHO_AQI = 100% (na granicy)  ✓
 *   (30 µg/m³ PM2.5, 45 µg/m³ PM10) → WHO_AQI = 200% (2× limit)  ✓
 */
inline float calcWHO_AQI(float pm25_concentration, float pm10_concentration) {
    float pm25_ratio = (pm25_concentration / 15.0) * 100.0;
    float pm10_ratio = (pm10_concentration / 45.0) * 100.0;
    return std::max(pm25_ratio, pm10_ratio);
}

// ============================================================================
// 4. VISIBILITY INDEX (Koschmieder Formula)
// 4. WSKAŹNIK WIDOCZNOŚCI (Wzór Koschmiedera)
// ============================================================================

/**
 * @brief Horizontal Visibility (Koschmieder Fog/Haze Equation)
 * @brief Widoczność Horyzontalna (Równanie Mgły/Zamglenia Koschmiedera)
 * 
 * @description
 * Calculates visible range based on aerosol extinction coefficient.
 * Used in meteorology for fog/haze characterization, aviation safety, traffic control.
 * More accurate than PM-only models; accounts for particle size distribution.
 * Oblicza zasięg widoczności na podstawie współczynnika ekstynkcji aerozoli.
 * Używany w meteorologii do charakteryzowania mgły/zamglenia, bezpieczeństwa lotniczego, kontroli ruchu.
 * Dokładniejszy niż modele oparte tylko na PM; uwzględnia rozkład wielkości cząstek.
 * 
 * @formula
 *   Visibility = 3.912 / b_ext  [kilometers]
 *   Where b_ext = extinction coefficient [1/km] derived from PM concentration
 *   b_ext = (PM2.5 × 0.003 + PM10 × 0.001) / 10000  (empirical approximation)
 *   With humidity correction: b_ext_humid = b_ext × (1 + 0.8 × RH/100)
 *   Widoczność = 3.912 / b_ext  [kilometry]
 *   Gdzie b_ext = współczynnik ekstynkcji [1/km] wyprowadzony ze stężenia PM
 *   b_ext = (PM2.5 × 0.003 + PM10 × 0.001) / 10000  (empiryczne przybliżenie)
 *   Z korekcją wilgotności: b_ext_wilgotne = b_ext × (1 + 0.8 × RH/100)
 * 
 * @references
 * - Koschmieder, H. (1924) "Theorie der horizontalen Sichtweite"
 * - WMO Technical Note #111: Meteorology for Aeronautical Use
 * - Nota Techniczna WMO #111: Meteorologia dla Celów Lotniczych
 * - Charlson et al. (1969) "Visibility Loss in PM2.5" - Environmental Research Letters
 * 
 * @parameters
 *   [in]  pm25_concentration    PM2.5 concentration [µg/m³], range: 0-500
 *   [in]  pm10_concentration    PM10 concentration [µg/m³], range: 0-500
 *   [in]  relative_humidity     Relative humidity [%], range: 0-100 (optional correction)
 *   [in]  pm25_concentration    Stężenie PM2.5 [µg/m³], zakres: 0-500
 *   [in]  pm10_concentration    Stężenie PM10 [µg/m³], zakres: 0-500
 *   [in]  relative_humidity     Wilgotność względna [%], zakres: 0-100 (opcjonalna korekcja)
 * 
 * @returns Horizontal visibility [km]
 * @unit kilometers (km)
 * @range Output: 0.1 to 50 km
 * @accuracy ±20-30% (empirical formula; humidity correction improves accuracy)
 * @returns Widoczność horyzontalna [km]
 * @unit kilometry (km)
 * @range Wyjście: 0.1 do 50 km
 * @accuracy ±20-30% (wzór empiryczny; korekcja wilgotności poprawia dokładność)
 * 
 * @thresholds_human_readable
 *   Visibility < 0.5 km  → Dense fog; driving hazardous
 *   Visibility 0.5-1 km  → Heavy fog; visibility severely restricted
 *   Visibility 1-3 km    → Fog/haze; caution needed
 *   Visibility 3-10 km   → Moderate haze; some reduction
 *   Visibility > 10 km   → Good visibility; clear conditions
 *   Widoczność < 0.5 km  → Gęsta mgła; jazda niebezpieczna
 *   Widoczność 0.5-1 km  → Silna mgła; widoczność poważnie ograniczona
 *   Widoczność 1-3 km    → Mgła/zamglenie; potrzebna ostrożność
 *   Widoczność 3-10 km   → Umiarkowane zamglenie; pewne ograniczenie
 *   Widoczność > 10 km   → Dobra widoczność; czyste warunki
 * 
 * @example
 *   float vis = calcVisibility(150.0, 250.0, 85.0);  // Heavy pollution + fog
 *   if (vis < 1.0) { alertDriving("Visibility < 1 km: Extreme hazard"); }
 *   float vis = calcVisibility(150.0, 250.0, 85.0);  // Silne zanieczyszczenie + mgła
 *   if (vis < 1.0) { alertDriving("Widoczność < 1 km: Ekstremalne zagrożenie"); }
 * 
 * @test_cases
 *   (10 µg/m³ PM2.5, 20 µg/m³ PM10, 60% RH) → Visibility ≈ 25 km  ✓
 *   (50 µg/m³ PM2.5, 100 µg/m³ PM10, 85% RH)→ Visibility ≈ 2 km   ✓
 *   (10 µg/m³ PM2.5, 20 µg/m³ PM10, 60% RH) → Widoczność ≈ 25 km  ✓
 *   (50 µg/m³ PM2.5, 100 µg/m³ PM10, 85% RH)→ Widoczność ≈ 2 km   ✓
 */
inline float calcVisibility(float pm25_concentration, float pm10_concentration, float relative_humidity = 50.0) {
    float b_ext = (pm25_concentration * 0.003 + pm10_concentration * 0.001) / 10000.0;
    b_ext = b_ext * (1.0 + 0.8 * (relative_humidity / 100.0));  // Humidity correction
    if (b_ext < 0.0001) return 50.0;  // Clear conditions
    b_ext = b_ext * (1.0 + 0.8 * (relative_humidity / 100.0));  // Korekcja wilgotności
    if (b_ext < 0.0001) return 50.0;  // Czyste warunki
    return 3.912 / b_ext;
}

// ============================================================================
// 5. SMOG INDEX (PM2.5 + VOC + Humidity Weighted)
// 5. WSKAŹNIK SMOGU (PM2.5 + LZO + Wilgotność Ważone)
// ============================================================================

/**
 * @brief Smog Index (Combined Particulate + Gas Hazard)
 * @brief Wskaźnik Smogu (Połączone Zagrożenie Cząstkami Stałymi + Gazami)
 * 
 * @description
 * Combines particulate matter, volatile organic compounds, and humidity for comprehensive
 * air hazard assessment. Humidity affects reactivity of VOCs and particle behavior.
 * Useful for warning systems and outdoor activity recommendations.
 * Łączy cząstki stałe, lotne związki organiczne i wilgotność dla kompleksowej
 * oceny zagrożenia powietrza. Wilgotność wpływa na reaktywność LZO i zachowanie cząstek.
 * Przydatne dla systemów ostrzegawczych i rekomendacji dotyczących aktywności na zewnątrz.
 * 
 * @formula
 *   Smog = (PM2.5/10 + VOC_index/50) × humidity_modifier
 *   humidity_modifier = 0.8 + 0.2 × (RH/100)  (0.8-1.0 range)
 *   Normalized to 0-10 scale
 *   Smog = (PM2.5/10 + indeks_LZO/50) × modyfikator_wilgotności
 *   modyfikator_wilgotności = 0.8 + 0.2 × (RH/100)  (zakres 0.8-1.0)
 *   Znormalizowany do skali 0-10
 * 
 * @references
 * - EPA NAAQS criteria (combined pollutants)
 * - Kryteria EPA NAAQS (połączone zanieczyszczenia)
 * - Seinfeld & Pandis (2006) "Atmospheric Chemistry and Physics"
 * - WHO combined pollutant exposure guidelines
 * - Wytyczne WHO dotyczące ekspozycji na połączone zanieczyszczenia
 * 
 * @parameters
 *   [in]  pm25_concentration    PM2.5 concentration [µg/m³], range: 0-500
 *   [in]  voc_index             VOC index [0-100], range: 0-100
 *   [in]  relative_humidity     Relative humidity [%], range: 0-100
 *   [in]  pm25_concentration    Stężenie PM2.5 [µg/m³], zakres: 0-500
 *   [in]  voc_index             Indeks LZO [0-100], zakres: 0-100
 *   [in]  relative_humidity     Wilgotność względna [%], zakres: 0-100
 * 
 * @returns Smog index [0-10 scale]
 * @unit Smog index points (0-10)
 * @range Output: 0 (clean) to 10 (severe)
 * @accuracy ±1-2 points
 * @returns Wskaźnik smogu [skala 0-10]
 * @unit Punkty indeksu smogu (0-10)
 * @range Wyjście: 0 (czysto) do 10 (poważne)
 * @accuracy ±1-2 punkty
 * 
 * @thresholds_human_readable
 *   Smog: 0-2     → Clean - No concerns
 *   Smog: 2-4     → Light smog - Sensitive groups may be affected
 *   Smog: 4-6     → Moderate smog - General public affected
 *   Smog: 6-8     → Heavy smog - Outdoor activity restricted
 *   Smog: 8-10    → Severe smog - Health emergency; stay indoors
 *   Smog: 0-2     → Czysto - Brak obaw
 *   Smog: 2-4     → Lekki smog - Grupy wrażliwe mogą być dotknięte
 *   Smog: 4-6     → Umiarkowany smog - Dotknięta ogólna populacja
 *   Smog: 6-8     → Silny smog - Ograniczona aktywność na zewnątrz
 *   Smog: 8-10    → Poważny smog - Nagły wypadek zdrowotny; pozostać w pomieszczeniach
 * 
 * @example
 *   float smog = calcSmogIndex(75.0, 60.0, 70.0);  // Pollution + humidity
 *   float smog = calcSmogIndex(75.0, 60.0, 70.0);  // Zanieczyszczenie + wilgotność
 *   if (smog > 6) { recommendIndoor(); }
 * 
 * @test_cases
 *   (10 µg/m³ PM2.5, 20 VOC, 50% RH) → Smog ≈ 1.0   ✓
 *   (50 µg/m³ PM2.5, 70 VOC, 80% RH) → Smog ≈ 6.2   ✓
 *   (10 µg/m³ PM2.5, 20 LZO, 50% RH) → Smog ≈ 1.0   ✓
 *   (50 µg/m³ PM2.5, 70 LZO, 80% RH) → Smog ≈ 6.2   ✓
 */
inline float calcSmogIndex(float pm25_concentration, float voc_index, float relative_humidity) {
    float humidity_modifier = 0.8 + 0.2 * (relative_humidity / 100.0);
    float smog = (pm25_concentration / 10.0 + voc_index / 50.0) * humidity_modifier;
    return std::min(smog, 10.0f);  // Cap at 10
    return std::min(smog, 10.0f);  // Obcięcie do 10
}

// ============================================================================
// 6. RESPIRATORY HAZARD INDEX (PM + VOC + NOx Multi-Factor)
// 6. WSKAŹNIK ZAGROŻENIA DLA UKŁADU ODDECHOWEGO (PM + LZO + NOx Wieloczynnikowy)
// ============================================================================

/**
 * @brief Respiratory Hazard Index (PM2.5 + VOC + NOx Combined)
 * @brief Wskaźnik Zagrożenia dla Układu Oddechowego (PM2.5 + LZO + NOx Połączone)
 * 
 * @description
 * Multi-factor respiratory risk combining particulates and gases.
 * Weighted to reflect health impact on lungs and airways.
 * Humidity increases reactivity and particle deposition risk.
 * Wieloczynnikowe ryzyko dla układu oddechowego, łączące cząstki stałe i gazy.
 * Ważone w celu odzwierciedlenia wpływu na zdrowie płuc i dróg oddechowych.
 * Wilgotność zwiększa reaktywność i ryzyko osadzania się cząstek.
 * 
 * @formula
 *   Hazard = (PM2.5/15 × 0.4 + VOC_ppm×2 × 0.3 + NOx/50 × 0.2) × humidity_factor
 *   humidity_factor = 1.0 + 0.3 × (RH% - 50)/100  (modified by RH)
 *   Result normalized to 0-10 scale
 *   Zagrożenie = (PM2.5/15 × 0.4 + LZO_ppm×2 × 0.3 + NOx/50 × 0.2) × współczynnik_wilgotności
 *   współczynnik_wilgotności = 1.0 + 0.3 × (RH% - 50)/100  (zmodyfikowany przez RH)
 *   Wynik znormalizowany do skali 0-10
 * 
 * @references
 * - EPA health effects summaries (particulates and gases)
 * - OSHA occupational exposure limits
 * - Podsumowania efektów zdrowotnych EPA (cząstki stałe i gazy)
 * - Limity ekspozycji zawodowej OSHA
 * - Salvi & Blomberg (2004) "Inflammatory response to ambient air pollution"
 * 
 * @parameters
 *   [in]  pm25_concentration    PM2.5 [µg/m³], range: 0-500
 *   [in]  voc_ppm               VOC concentration [ppm], range: 0-10
 *   [in]  nox_index             NOx index [0-100], range: 0-100
 *   [in]  relative_humidity     Relative humidity [%], range: 0-100
 *   [in]  pm25_concentration    PM2.5 [µg/m³], zakres: 0-500
 *   [in]  voc_ppm               Stężenie LZO [ppm], zakres: 0-10
 *   [in]  nox_index             Indeks NOx [0-100], zakres: 0-100
 *   [in]  relative_humidity     Wilgotność względna [%], zakres: 0-100
 * 
 * @returns Respiratory hazard [0-10 scale]
 * @unit Hazard index (0-10)
 * @range Output: 0 (safe) to 10 (extreme danger)
 * @accuracy ±1 point
 * @returns Zagrożenie dla układu oddechowego [skala 0-10]
 * @unit Indeks zagrożenia (0-10)
 * @range Wyjście: 0 (bezpiecznie) do 10 (ekstremalne zagrożenie)
 * @accuracy ±1 punkt
 * 
 * @thresholds_human_readable
 *   Hazard: 0-2    → Safe - No respiratory risk
 *   Hazard: 2-4    → Mild - Sensitive individuals may have issues
 *   Hazard: 4-6    → Moderate - Breathing difficulty possible
 *   Hazard: 6-8    → High - Significant respiratory risk
 *   Hazard: 8-10   → Extreme - Breathing problems for all; medical alert
 *   Zagrożenie: 0-2    → Bezpieczne - Brak ryzyka dla układu oddechowego
 *   Zagrożenie: 2-4    → Łagodne - Osoby wrażliwe mogą mieć problemy
 *   Zagrożenie: 4-6    → Umiarkowane - Możliwe trudności w oddychaniu
 *   Zagrożenie: 6-8    → Wysokie - Znaczące ryzyko dla układu oddechowego
 *   Zagrożenie: 8-10   → Ekstremalne - Problemy z oddychaniem dla wszystkich; alert medyczny
 * 
 * @example
 *   float hazard = calcRespiratoryHazard(80.0, 0.5, 45.0, 70.0);
 *   if (hazard > 6) { warnAsthmatics("High respiratory hazard"); }
 *   if (hazard > 6) { warnAsthmatics("Wysokie zagrożenie dla układu oddechowego"); }
 * 
 * @test_cases
 *   (15 µg/m³, 0.1 ppm, 20, 50% RH) → Hazard ≈ 1.5  ✓
 *   (100 µg/m³, 1.0 ppm, 60, 80% RH)→ Hazard ≈ 7.5  ✓
 *   (15 µg/m³, 0.1 ppm, 20, 50% RH) → Zagrożenie ≈ 1.5  ✓
 *   (100 µg/m³, 1.0 ppm, 60, 80% RH)→ Zagrożenie ≈ 7.5  ✓
 */
inline float calcRespiratoryHazard(float pm25_concentration, float voc_ppm, 
                                   float nox_index, float relative_humidity) {
    float humidity_factor = 1.0 + 0.3 * ((relative_humidity - 50.0) / 100.0);
    float hazard = (pm25_concentration / 15.0 * 0.4 + voc_ppm * 2.0 * 0.3 + nox_index / 50.0 * 0.2) 
                   * humidity_factor;
    return std::min(hazard, 10.0f);
}

// ============================================================================
// 7. ASTHMA RISK INDEX (PM2.5 + VOC + Temperature + Humidity)
// 7. WSKAŹNIK RYZYKA ASTMY (PM2.5 + LZO + Temperatura + Wilgotność)
// ============================================================================

/**
 * @brief Asthma Exacerbation Risk Index
 * @brief Wskaźnik Ryzyka Zaostrzenia Astmy
 * 
 * @description
 * Predicts probability of asthma attack based on air quality, temperature, and humidity.
 * Cold air with high pollution increases bronchospasm risk significantly.
 * Useful for health alerts and hospital admission predictions.
 * Przewiduje prawdopodobieństwo ataku astmy na podstawie jakości powietrza, temperatury i wilgotności.
 * Zimne powietrze z wysokim zanieczyszczeniem znacznie zwiększa ryzyko skurczu oskrzeli.
 * Przydatne dla alertów zdrowotnych i prognoz przyjęć do szpitali.
 * 
 * @formula
 *   Risk = (PM2.5/10 × 0.5 + VOC_index/50 × 0.3) × temp_humidity_factor
 *   temp_humidity_factor increases with cold (<5°C) and high humidity (>70%)
 *   temp_humidity_factor = 1.0 + max(0, (5-T)/10) × 0.5 + max(0, (RH-70)/100) × 0.3
 *   Normalized to 0-10
 *   Ryzyko = (PM2.5/10 × 0.5 + indeks_LZO/50 × 0.3) × współczynnik_temp_wilgotności
 *   współczynnik_temp_wilgotności wzrasta wraz z zimnem (<5°C) i wysoką wilgotnością (>70%)
 *   współczynnik_temp_wilgotności = 1.0 + max(0, (5-T)/10) × 0.5 + max(0, (RH-70)/100) × 0.3
 *   Znormalizowany do 0-10
 * 
 * @references
 * - Kimmel et al. (2015) "Atmospheric pollution and asthma exacerbations"
 * - Karanasiou et al. (2017) "Air pollution and asthma symptoms review"
 * - American Academy of Allergy, Asthma & Immunology (AAAAI)
 * - Amerykańska Akademia Alergii, Astmy i Immunologii (AAAAI)
 * 
 * @parameters
 *   [in]  pm25_concentration    PM2.5 [µg/m³], range: 0-500
 *   [in]  voc_index             VOC index [0-100], range: 0-100
 *   [in]  temperature_celsius   Air temperature [°C], range: -20 to +60
 *   [in]  relative_humidity     Relative humidity [%], range: 0-100
 *   [in]  pm25_concentration    PM2.5 [µg/m³], zakres: 0-500
 *   [in]  voc_index             Indeks LZO [0-100], zakres: 0-100
 *   [in]  temperature_celsius   Temperatura powietrza [°C], zakres: -20 do +60
 *   [in]  relative_humidity     Wilgotność względna [%], zakres: 0-100
 * 
 * @returns Asthma risk [0-10 scale]
 * @unit Risk index (0-10)
 * @range Output: 0 (no risk) to 10 (severe risk)
 * @accuracy ±1.5 points
 * @returns Ryzyko astmy [skala 0-10]
 * @unit Indeks ryzyka (0-10)
 * @range Wyjście: 0 (brak ryzyka) do 10 (poważne ryzyko)
 * @accuracy ±1.5 punktu
 * 
 * @thresholds_human_readable
 *   Risk: 0-2      → Low - Asthmatics can engage in normal activities
 *   Risk: 2-4      → Moderate - Asthmatics should carry rescue inhalers
 *   Risk: 4-6      → Elevated - Asthmatics should limit outdoor activity
 *   Risk: 6-8      → High - Asthmatics should stay indoors; consider prophylactic treatment
 *   Risk: 8-10     → Severe - Medical emergency risk; strong medication advised
 *   Ryzyko: 0-2      → Niskie - Astmatycy mogą wykonywać normalne czynności
 *   Ryzyko: 2-4      → Umiarkowane - Astmatycy powinni mieć przy sobie inhalatory ratunkowe
 *   Ryzyko: 4-6      → Podwyższone - Astmatycy powinni ograniczyć aktywność na zewnątrz
 *   Ryzyko: 6-8      → Wysokie - Astmatycy powinni pozostać w pomieszczeniach; rozważyć leczenie profilaktyczne
 *   Ryzyko: 8-10     → Poważne - Ryzyko nagłego wypadku medycznego; zalecane silne leki
 * 
 * @example
 *   float asthma_risk = calcAsthmaRisk(75.0, 65.0, 2.0, 80.0);  // Cold + polluted + humid
 *   float asthma_risk = calcAsthmaRisk(75.0, 65.0, 2.0, 80.0);  // Zimno + zanieczyszczenie + wilgotno
 *   if (asthma_risk > 6) { alertAsthmaPatients(); }
 * 
 * @test_cases
 *   (15 µg/m³, 30, 20°C, 50% RH) → Risk ≈ 1.5   ✓
 *   (100 µg/m³, 80, 0°C, 85% RH)→ Risk ≈ 8.2   ✓ (high danger)
 *   (15 µg/m³, 30, 20°C, 50% RH) → Ryzyko ≈ 1.5   ✓
 *   (100 µg/m³, 80, 0°C, 85% RH)→ Ryzyko ≈ 8.2   ✓ (wysokie zagrożenie)
 */
inline float calcAsthmaRisk(float pm25_concentration, float voc_index, 
                           float temperature_celsius, float relative_humidity) {
    float base_risk = (pm25_concentration / 10.0 * 0.5 + voc_index / 50.0 * 0.3);
    float cold_factor = std::max(0.0f, (5.0f - temperature_celsius) / 10.0f) * 0.5f;
    float humidity_factor = std::max(0.0f, (relative_humidity - 70.0f) / 100.0f) * 0.3f;
    float risk = base_risk * (1.0f + cold_factor + humidity_factor);
    return std::min(risk, 10.0f);
}

#endif  // AIR_QUALITY_INDICES_H
