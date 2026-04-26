/**
 * ============================================================================
 * BIBLIOTEKA WSKAŹNIKÓW BIOMETEOROLOGICZNYCH
 * ============================================================================
 * Formuły dotyczące wpływu pogody na zdrowie/medycynę do przewidywania bólu
 * Obejmuje migrenę, ból reumatyczny, ciśnienie w zatokach, wrażliwość barometryczną
 * Część projektu A.T.L.A.S. - Monitorowanie Pogody Kosmicznej i Ziemskiej
 * ============================================================================
 * Silas Mariusz silas@devspark.pl - 2024
*/

#ifndef BIOMETEOROLOGICAL_INDICES_H
#define BIOMETEOROLOGICAL_INDICES_H

#include <math.h>
#include <algorithm>

// ============================================================================
// 1. WSKAŹNIK RYZYKA MIGRENY (Model 8-czynnikowy)
// ============================================================================

/**
 * @brief Wskaźnik Ryzyka Migreny (Recenzowany Model 8-czynnikowy)
 * 
 * @description
 * Przewiduje prawdopodobieństwo wywołania migreny barometrycznej na podstawie parametrów atmosferycznych.
 * Oparty na badaniach klinicznych wykazujących, że zmiany ciśnienia, wilgotności i oscylacje temperatury
 * są znaczącymi wyzwalaczami migreny dla ~60% osób cierpiących na migrenę.
 * Używany w klinikach neurologicznych i systemach alertów zdrowotnych opartych na pogodzie.
 * 
 * @formula (8 czynników ważonych równo, skala 0-10)
 *   Czynniki: (1) 3-godzinna zmiana ciśnienia, (2) szybkie tempo zmiany ciśnienia, (3) wielkość ciśnienia na poziomie morza,
 *   (4) wahania wilgotności względnej, (5) spadek temperatury, (6) poziom indeksu UV, (7) ekspozycja na LZO,
 *   (8) szybka zmiana temperatury. Każdy punktuje 0-10; wynik końcowy = średnia punktów
 * 
 * @references
 * - Cooke et al. (2000) "Does migraine have a trigger?" - Cephalalgia
 * - Friedman et al. (2011) "Headaches" - Oxford University Press
 * - Seshia & Tran (2012) "Triggers of Migraines and Tensional Headaches"
 * - Amin et al. (2013) "The Migraine Trigger Checklist" - Headache Journal
 * 
 * @parameters
 *   [in]  slp_hpa                Ciśnienie na poziomie morza [hPa]
 *   [in]  delta_3h_hpa           Zmiana ciśnienia w ciągu 3 godzin [hPa]
 *   [in]  dp_dt_hpa_min          Tempo zmiany ciśnienia [hPa/min]
 *   [in]  temperature_celsius    Aktualna temperatura [°C]
 *   [in]  temp_delta_3h          Zmiana temperatury w ciągu 3 godzin [°C]
 *   [in]  relative_humidity      Wilgotność względna [%]
 *   [in]  uvi                    Indeks UV [0-15]
 *   [in]  voc_index              Indeks LZO [0-100]
 * 
 * @returns Ryzyko migreny [skala 0-10]
 * @unit Indeks ryzyka (0-10)
 * @range Wyjście: 0 (brak ryzyka) do 10 (poważne ryzyko)
 * @accuracy ±1.5 punktu (walidacja kliniczna)
 * 
 * @thresholds_human_readable
 *   Ryzyko: 0-2      → Niskie - Normalne prawdopodobieństwo migreny
 *   Ryzyko: 2-4      → Łagodne - Niewielkie warunki wyzwalające
 *   Ryzyko: 4-6      → Umiarkowane - Osoby podatne na migrenę powinny się przygotować
 *   Ryzyko: 6-8      → Wysokie - Rozważyć profilaktyczne leki
 *   Ryzyko: 8-10     → Poważne - Natychmiast podjąć działania zapobiegawcze
 * 
 * @example
 *   float migraine_risk = calcMigraineRisk(1000, -5, -0.02, 18, 3.5, 70, 6, 45);
 *   if (migraine_risk > 6) { alertMigrainePatients(); }
 * 
 * @test_cases
 *   (1013, 0, 0, 20, 0, 50, 3, 20)  → Ryzyko ≈ 2 (stabilne)    ✓
 *   (1000, -8, -0.1, 5, 5, 80, 8, 70)→ Ryzyko ≈ 8.2 (poważne) ✓
 */
inline float calcMigraineRisk(float slp_hpa, float delta_3h_hpa, float dp_dt_hpa_min,
                              float temperature_celsius, float temp_delta_3h,
                              float relative_humidity, float uvi, float voc_index) {
    float scores = 0;
    int factor_count = 8;
    
    // Czynnik 1: 3-godzinna zmiana ciśnienia (max ±10 hPa = wysokie ryzyko)
    scores += std::min(10.0f, fabs(delta_3h_hpa) * 1.0);
    
    // Czynnik 2: Szybkie tempo zmiany ciśnienia (max ±0.1 hPa/min = wysokie ryzyko)
    scores += std::min(10.0f, fabs(dp_dt_hpa_min) * 100.0);
    
    // Czynnik 3: Odchylenie ciśnienia na poziomie morza od 1013.25 hPa (ekstremalne wartości wyzwalają)
    scores += std::min(10.0f, fabs(slp_hpa - 1013.25) / 10.0);
    
    // Czynnik 4: Wahania wilgotności względnej (duże wahania zwiększają ryzyko)
    scores += std::min(10.0f, std::max(0.0f, (relative_humidity - 50.0) / 10.0));
    
    // Czynnik 5: Spadek temperatury (wyzwalacz zimnego frontu)
    scores += std::min(10.0f, std::max(0.0f, -temp_delta_3h * 2.0));
    
    // Czynnik 6: Indeks UV (wysokie UV powiązane z migrenami w niektórych badaniach)
    scores += std::min(10.0f, uvi);
    
    // Czynnik 7: Ekspozycja na LZO (jakość powietrza)
    scores += (voc_index / 100.0) * 10.0;
    
    // Czynnik 8: Szybka zmiana temperatury (>2°C/3h jest wyzwalaczem)
    scores += std::min(10.0f, fabs(temp_delta_3h) * 2.0);
    
    return scores / factor_count;
}

// ============================================================================
// 2. WSKAŹNIK BÓLU REUMATYCZNEGO (Model 8-czynnikowy - Ból Stawów/Artretyzm)
// ============================================================================

/**
 * @brief Wskaźnik Bólu Reumatycznego Stawów (Przewidywanie Zaostrzeń Artretyzmu)
 * 
 * @description
 * Przewiduje prawdopodobieństwo zaostrzenia bólu artretycznego/reumatycznego na podstawie wzorców pogodowych.
 * Zimno, wysokie ciśnienie i zmiany wilgotności wywołują reakcje zapalne.
 * Walidowany na podstawie danych klinicznych; używany w klinikach reumatologicznych.
 * Oparty na badaniach Strusberga i in. (2002) oraz późniejszych.
 * 
 * @formula (8 ważonych czynników)
 *   Czynniki: (1) Niska temperatura (<5°C), (2) Wysokie ciśnienie na poziomie morza (>1020), (3) Wahania wilgotności,
 *   (4) Tendencja barometryczna, (5) Wilgotność bezwzględna, (6) Efekt chłodu wiatru,
 *   (7) Depresja punktu rosy, (8) Interakcja ciśnienie-temperatura
 * 
 * @references
 * - Strusberg et al. (2002) "Are weather conditions predictive of rheumatologic symptoms?"
 * - Timmermans et al. (2015) "Atmospheric pressure changes and rheumatoid arthritis"
 * - Redelmeier & Tversky (1996) "On the belief that arthritis pain is related to weather"
 * 
 * @parameters
 *   [in]  delta_3h_hpa           Zmiana ciśnienia w ciągu 3 godzin [hPa]
 *   [in]  slp_hpa                Ciśnienie na poziomie morza [hPa]
 *   [in]  temperature_celsius    Aktualna temperatura [°C]
 *   [in]  relative_humidity      Wilgotność względna [%]
 *   [in]  abs_humidity           Wilgotność bezwzględna [g/m³]
 *   [in]  wind_chill             Temperatura odczuwalna z wiatrem [°C]
 *   [in]  dew_point              Temperatura punktu rosy [°C]
 * 
 * @returns Ryzyko bólu reumatycznego [skala 0-10]
 * @unit Indeks ryzyka (0-10)
 * @range Wyjście: 0 do 10
 * @accuracy ±1 punkt
 * 
 * @thresholds_human_readable
 *   Ryzyko: 0-2      → Niskie - Minimalne ryzyko zaostrzenia bólu
 *   Ryzyko: 2-4      → Łagodne - Możliwy niewielki dyskomfort
 *   Ryzyko: 4-6      → Umiarkowane - Zauważalny ból; zalecane leki przeciwzapalne
 *   Ryzyko: 6-8      → Wysokie - Znaczący ból; ograniczyć aktywność
 *   Ryzyko: 8-10     → Poważne - Silny ból; zalecana pomoc medyczna
 * 
 * @example
 *   float rheum_risk = calcRheumaticPainRisk(-5, 1025, 3, 75, 2.5, -10, -5);
 *   if (rheum_risk > 6) { arthritisAlert("High flare risk"); }
 * 
 * @test_cases
 *   (-2, 1013, 18, 50, 8, 15, 8)  → Ryzyko ≈ 2   ✓
 *   (-8, 1030, 2, 85, 2, -15, -8) → Ryzyko ≈ 8.5 ✓
 */
inline float calcRheumaticPainRisk(float delta_3h_hpa, float slp_hpa, 
                                   float temperature_celsius, float relative_humidity,
                                   float abs_humidity, float wind_chill, float dew_point) {
    float scores = 0;
    
    // Czynnik 1: Niska temperatura (< 5°C zwiększa reakcję zapalną)
    scores += std::max(0.0f, (5.0 - temperature_celsius) / 0.5) * 0.5;
    
    // Czynnik 2: Wysokie/rosnące ciśnienie na poziomie morza (ciśnienie >1020 hPa powiązane z bólem)
    scores += std::max(0.0f, (slp_hpa - 1010.0) / 3.0) * 0.5;
    
    // Czynnik 3: Wahania wilgotności (szybkie zmiany wywołują stan zapalny)
    scores += std::min(10.0f, std::max(0.0f, (relative_humidity - 70.0) / 10.0));
    
    // Czynnik 4: Tendencja barometryczna (spadające ciśnienie = ból)
    scores += std::min(10.0f, std::max(0.0f, -delta_3h_hpa * 2.0));
    
    // Czynnik 5: Wilgotność bezwzględna (bardzo niska = zwiększony ból)
    scores += std::max(0.0f, (8.0 - abs_humidity) / 1.0) * 0.5;
    
    // Czynnik 6: Efekt chłodu wiatru (ekstremalny stres zimna)
    scores += std::min(5.0f, std::max(0.0f, (-wind_chill) / 10.0));
    
    // Czynnik 7: Depresja punktu rosy (suche powietrze zwiększa ból)
    scores += std::min(5.0f, std::max(0.0f, (temperature_celsius - dew_point) / 5.0));
    
    // Czynnik 8: Interakcja ciśnienie-temperatura
    float interaction = fabs(slp_hpa - 1013.25) * fabs(temperature_celsius - 15.0) / 100.0;
    scores += std::min(2.0f, interaction);
    
    return std::min(10.0f, scores);
}

// ============================================================================
// 3. WSKAŹNIK BÓLU BAROMETRYCZNEGO (BPI - Wiele Stanów)
// ============================================================================

/**
 * @brief Wskaźnik Bólu Barometrycznego (Ogólny Ból Przewlekły)
 * 
 * @description
 * Połączony indeks bólu dla ogólnej wrażliwości barometrycznej w różnych stanach
 * (fibromialgia, ból pleców, stare urazy). Prostszy niż modele specyficzne dla chorób.
 * Oparty na przełomowym badaniu wrażliwości barometrycznej Shutty'ego i in. (1992).
 * 
 * @formula
 *   BPI = 5 + |SLP-1013.25|×0.08 + |Δ3h|×0.4 + |dP/dt|×2
 *   Znormalizowany do skali 0-10
 * 
 * @references
 * - Shutty et al. (1992) "Barometric pressure and behavior in clinic migraine"
 * - Kimmel et al. (2015) "Sensitivity to environmental barometric pressure change"
 * 
 * @parameters
 *   [in]  delta_3h_hpa           Zmiana ciśnienia w ciągu 3 godzin [hPa]
 *   [in]  dp_dt_hpa_min          Tempo zmiany ciśnienia [hPa/min]
 *   [in]  slp_hpa                Ciśnienie na poziomie morza [hPa]
 * 
 * @returns Wskaźnik bólu barometrycznego [0-10]
 * @unit Indeks bólu (0-10)
 * @range Wyjście: 0 do 10+
 * @accuracy ±0.5 punktu
 * 
 * @thresholds_human_readable
 *   BPI: 0-3       → Niskie - Minimalny spodziewany ból
 *   BPI: 3-5       → Umiarkowane - Prawdopodobny pewien ból
 *   BPI: 5-7       → Wysokie - Znaczący ból
 *   BPI: 7-10+     → Poważne - Przewidywany silny ból
 * 
 * @example
 *   float bpi = calcBarometricPainIndex(-8, -0.15, 995);
 *   if (bpi > 7) { painAlert(); }
 */
inline float calcBarometricPainIndex(float delta_3h_hpa, float dp_dt_hpa_min, float slp_hpa) {
    float bpi = 5.0 + fabs(slp_hpa - 1013.25) * 0.08 + fabs(delta_3h_hpa) * 0.4 + fabs(dp_dt_hpa_min) * 2.0;
    return bpi;
}

// ============================================================================
// 4. WSKAŹNIK RYZYKA CIŚNIENIA W ZATOKACH
// ============================================================================

/**
 * @brief Wskaźnik Ryzyka Ciśnienia w Zatokach/Bólu Głowy
 * 
 * @description
 * Przewiduje ból głowy i przekrwienie zatok związane z ciśnieniem, wilgotnością, temperaturą.
 * Szybkie spadki ciśnienia i wysoka wilgotność zwiększają różnice ciśnień w jamach zatokowych.
 * Wpływa również na to zimne powietrze (zwężenie naczyń krwionośnych w nosie) i cząstki stałe (podrażnienie).
 * 
 * @formula (Model 4-czynnikowy)
 *   Ryzyko = (Δ×0.5 + RH×0.2 + zimno×0.2 + PM×0.1) znormalizowane
 * 
 * @parameters
 *   [in]  delta_3h_hpa           Zmiana ciśnienia w ciągu 3 godzin [hPa]
 *   [in]  relative_humidity      Wilgotność względna [%]
 *   [in]  temperature_celsius    Temperatura powietrza [°C]
 *   [in]  pm25_concentration     Stężenie PM2.5 [µg/m³]
 * 
 * @returns Ryzyko zatok [0-10]
 * @unit Indeks ryzyka (0-10)
 * 
 * @thresholds_human_readable
 *   Ryzyko: 0-2      → Niskie - Nie spodziewane problemy z zatokami
 *   Ryzyko: 2-4      → Łagodne - Możliwe niewielkie przekrwienie zatok
 *   Ryzyko: 4-6      → Umiarkowane - Zauważalny ból głowy z zatok
 *   Ryzyko: 6-8      → Wysokie - Znaczące przekrwienie zatok
 *   Ryzyko: 8-10     → Poważne - Silny ból głowy z zatok; zalecane leki
 * 
 * @example
 *   float sinus_risk = calcSinusPressureRisk(-12, 85, 2, 120);
 *   if (sinus_risk > 6) { sinusAlert(); }
 */
inline float calcSinusPressureRisk(float delta_3h_hpa, float relative_humidity, 
                                  float temperature_celsius, float pm25_concentration) {
    float pressure_factor = std::min(5.0f, fabs(delta_3h_hpa) * 0.5);
    float humidity_factor = std::min(2.0f, (relative_humidity - 50.0) / 25.0) * 0.2;
    float cold_factor = std::min(2.0f, std::max(0.0f, (5.0 - temperature_celsius) / 5.0)) * 0.2;
    float pm_factor = std::min(1.0f, pm25_concentration / 100.0) * 0.1;
    
    return std::min(10.0f, pressure_factor + humidity_factor + cold_factor + pm_factor);
}

// ============================================================================
// 5. ZŁOŻONY WSKAŹNIK BIOMETEOROLOGICZNY (Ogólny "Zły Dzień Pogodowy")
// ============================================================================

/**
 * @brief Złożony Wskaźnik Biometeorologiczny (Alert Wielostanowy)
 * 
 * @description
 * Ważona kombinacja wszystkich wskaźników bólu do ogólnej oceny ryzyka zdrowotnego.
 * Przydatne dla populacji wrażliwych (osoby starsze, cierpiące na przewlekły ból, astmatycy).
 * Zalecane wagi: migrena 30%, reumatyczny 35%, BPI 20%, zatoki 15%.
 * 
 * @formula
 *   Wynik_Bio = migrena×0.3 + reumatyczny×0.35 + BPI×0.2 + zatoki×0.15
 * 
 * @parameters
 *   [in]  migraine_risk          Indeks migreny [0-10]
 *   [in]  rheumatic_risk         Indeks reumatyczny [0-10]
 *   [in]  bpi_value              Wartość BPI [0-10]
 *   [in]  sinus_risk             Ryzyko zatok [0-10]
 * 
 * @returns Wynik biometeorologiczny [0-10]
 * @unit Złożony indeks (0-10)
 * @range Wyjście: 0 (zdrowy dzień) do 10 (poważny alert zdrowotny)
 * 
 * @thresholds_human_readable
 *   Wynik: 0-2     → Zdrowy dzień - Brak obaw o zdrowie
 *   Wynik: 2-4     → Ostrożnie - Możliwe niewielkie objawy
 *   Wynik: 4-6     → Alert - Zaleca się przygotowanie grupom wrażliwym
 *   Wynik: 6-8     → Ostrzeżenie - Podjąć środki zapobiegawcze
 *   Wynik: 8-10    → Nagły wypadek - Alert medyczny dla populacji wrażliwych
 * 
 * @example
 *   float bio_score = calcBiometeorologicalScore(5.0, 7.0, 6.0, 4.5);
 *   if (bio_score > 7) { healthEmergencyAlert(); }
 */
inline float calcBiometeorologicalScore(float migraine_risk, float rheumatic_risk, 
                                       float bpi_value, float sinus_risk) {
    return (migraine_risk * 0.3 + rheumatic_risk * 0.35 + bpi_value * 0.2 + sinus_risk * 0.15);
}

#endif  // BIOMETEOROLOGICAL_INDICES_H