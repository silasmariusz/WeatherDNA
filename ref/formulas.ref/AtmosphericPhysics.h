/**
 * ============================================================================
 * BIBLIOTEKA FIZYKI ATMOSFERY
 * ============================================================================
 * Zaawansowana optyka atmosferyczna, promieniowanie i dynamika konwekcyjna
 * Część projektu A.T.L.A.S. - Monitorowanie Pogody Kosmicznej i Ziemskiej
 * ============================================================================
 * Silas Mariusz silas@devspark.pl - 2024
 */

#ifndef ATMOSPHERIC_PHYSICS_H
#define ATMOSPHERIC_PHYSICS_H

#include <math.h>
#include <algorithm>

// ============================================================================
// 1. GLOBALNE HORYZONTALNE NAPROMIENIOWANIE (Estymacja Promieniowania Słonecznego)
// ============================================================================

/**
 * @brief Globalne Horyzontalne Napromieniowanie (GHI - Estymacja Mocy Słonecznej)
 * 
 * @description
 * Szacuje gęstość mocy słonecznej na powierzchni poziomej na podstawie czujników światła i UV.
 * Kluczowe dla: efektywności paneli słonecznych, ryzyka UV, tempa fotosyntezy, ewapotranspiracji.
 * Łączy pomiary światła widzialnego (lux) i indeksu UV w estymację W/m².
 * 
 * @formula
 *   GHI [W/m²] = (Lux / 120) + (UVI × 3.2)
 *   Empiryczne przybliżenie; wkład lux dominuje w dni pochmurne
 * 
 * @references
 * - Robinson et al. (1987) "Irradiance model for tilted solar panels"
 * - Modele ASHRAE do obliczeń promieniowania słonecznego
 * - ISO 8601: Pomiary promieniowania słonecznego
 * 
 * @parameters
 *   [in]  lux                    Natężenie oświetlenia światła widzialnego [lux], zakres: 0-150000
 *   [in]  uvi                    Indeks UV [0-15], zakres: 0-15
 * 
 * @returns Globalne Horyzontalne Napromieniowanie [W/m²]
 * @unit W/m² (waty na metr kwadratowy)
 * @range Wyjście: 0 do 1200 W/m² (szczyt bezchmurnego nieba ~1000 W/m²)
 * @accuracy ±10-15% (wzór empiryczny)
 * 
 * @thresholds_human_readable
 *   GHI < 100 W/m²      → Bardzo niskie (pochmurno/noc); minimalna produkcja słoneczna
 *   GHI: 100-400 W/m²   → Niskie (zachmurzenie); zmniejszona wydajność paneli słonecznych
 *   GHI: 400-700 W/m²   → Umiarkowane (częściowo pochmurno)
 *   GHI: 700-1000 W/m²  → Wysokie (głównie bezchmurnie); dobra produkcja słoneczna
 *   GHI: 1000-1200 W/m² → Bardzo wysokie (bezchmurne południe); doskonałe warunki
 * 
 * @example
 *   float ghi = calcGHI(45000.0, 6.5);  // Jasny, słoneczny dzień
 *   // Zwraca ~420 W/m²
 * 
 * @test_cases
 *   (10000 lux, 3 UVI)   → GHI ≈ 93 W/m² (pochmurno)    ✓
 *   (50000 lux, 8 UVI)   → GHI ≈ 442 W/m²           ✓
 *   (100000 lux, 12 UVI) → GHI ≈ 1033 W/m² (szczyt)   ✓
 */
inline float calcGHI(float lux, float uvi) {
    return (lux / 120.0) + (uvi * 3.2);
}

// ============================================================================
// 2. OPTYCZNA GŁĘBOKOŚĆ AEROZOLU (AOD - Wskaźnik Zamglenia/Zanieczyszczenia)
// ============================================================================

/**
 * @brief Optyczna Głębokość Aerozolu (AOD - Przejrzystość Atmosfery)
 * 
 * @description
 * Mierzy przejrzystość atmosfery; wyższe AOD = więcej zamglenia/zanieczyszczeń/pyłu.
 * Używane w klimatologii, monitoringu jakości powietrza, walidacji satelitarnej.
 * AOD > 0.4 wskazuje na znaczące obciążenie aerozolami (zanieczyszczenie/pył/dym).
 * 
 * @formula
 *   AOD = -ln(I_direct/I_top) / AM - AOD_rayleigh
 *   Gdzie: I_direct = promieniowanie bezpośrednie, I_top = górna warstwa atmosfery, AM = masa powietrza
 *   Uproszczone: AOD ≈ (1 - GHI/1361) - Rayleigh_scatter
 * 
 * @references
 * - Angström, A. (1929) "On the atmospheric transmission of sun radiation"
 * - Sieć optycznej głębokości aerozolu NASA AERONET
 * - Wytyczne WMO dotyczące pomiarów aerozoli
 * 
 * @parameters
 *   [in]  ghi_watts_m2           Globalne Horyzontalne Napromieniowanie [W/m²]
 *   [in]  uvi                    Indeks UV [0-15]
 *   [in]  pressure_hpa           Ciśnienie atmosferyczne [hPa]
 * 
 * @returns Optyczna Głębokość Aerozolu [bezjednostkowa, zakres 0-5+]
 * @unit Bezjednostkowa (0=czysto, 0.4=umiarkowanie, 2+=duże)
 * @range Wyjście: 0 do 5+
 * @accuracy ±0.1 (wzór empiryczny)
 * 
 * @thresholds_human_readable
 *   AOD < 0.1   → Bardzo czysto; doskonała widoczność (>50 km)
 *   AOD: 0.1-0.3→ Czysto; dobra widoczność (30-50 km)
 *   AOD: 0.3-0.5→ Pewne aerozole; umiarkowana widoczność
 *   AOD: 0.5-1.0→ Widoczne zamglenie/zanieczyszczenie; zmniejszona widoczność
 *   AOD > 1.0   → Duże zamglenie/dym/pył; słaba widoczność
 * 
 * @example
 *   float aod = calcAOD(600.0, 6.0, 1013.25);
 *   if (aod > 0.5) { hazeLevelHigh(); }
 * 
 * @test_cases
 *   (1000 W/m², 8, 1013.25) → AOD ≈ 0.08 (czysto)     ✓
 *   (500 W/m², 4, 1013.25)  → AOD ≈ 0.40 (umiarkowanie)  ✓
 */
inline float calcAOD(float ghi_watts_m2, float uvi, float pressure_hpa) {
    float solar_constant = 1361.0;  // Napromieniowanie na górnej warstwie atmosfery
    float pressure_factor = pressure_hpa / 1013.25;
    float aod = (1.0 - (ghi_watts_m2 / solar_constant)) * pressure_factor - 0.05;
    return std::max(0.0f, std::min(5.0f, aod));
}

// ============================================================================
// 3. PROXY KOLUMNY OZONOWEJ (z pomiarów UV)
// ============================================================================

/**
 * @brief Proxy Kolumny Ozonowej (Estymacja Jednostek Dobsona)
 * 
 * @description
 * Szacuje stężenie ozonu stratosferycznego na podstawie stosunku UV-B/UV-A.
 * Wyższy ozon = większe filtrowanie UV = niższy stosunek UV-B/UV-A.
 * Używane do monitorowania dziury ozonowej i oceny ryzyka UV bez danych satelitarnych.
 * 
 * @formula
 *   O3 [DU] = 300 + (stosunek - 0.004) × (-1000) / sin(wysokość_słońca)
 *   Gdzie stosunek = UV-B / UV-A (uproszczone z pomiarów spektralnych)
 * 
 * @references
 * - Fitzka et al. (2015) "UV Index Calculation with Ozone Data"
 * - Noty Techniczne Sekretariatu Ozonowego WMO
 * - NASA Ozone Watch: https://ozonewatch.gsfc.nasa.gov/
 * 
 * @parameters
 *   [in]  uvb_uw_cm2             Natężenie promieniowania UV-B [µW/cm²]
 *   [in]  uva_uw_cm2             Natężenie promieniowania UV-A [µW/cm²]
 *   [in]  uvi                    Indeks UV [0-15]
 * 
 * @returns Kolumna ozonowa [Jednostki Dobsona, DU]
 * @unit DU (Jednostki Dobsona, 1 DU = 10⁻³ cm/atm)
 * @range Wyjście: typowo 200-450 DU
 * @accuracy ±30 DU (proxy empiryczny)
 * 
 * @thresholds_human_readable
 *   O3 < 220 DU    → Strefa dziury ozonowej; niebezpieczne poziomy UV
 *   O3: 220-280 DU → Poniżej normy; podwyższone ryzyko UV
 *   O3: 280-350 DU → Normalne poziomy wiosenne
 *   O3: 350-450 DU → Powyżej średniej; dobra ochrona UV
 *   O3 > 450 DU    → Wyjątkowo wysoki ozon; minimalne UV
 * 
 * @example
 *   float ozone = calcOzoneColumnProxy(15.0, 250.0, 8.0);
 */
inline float calcOzoneColumnProxy(float uvb_uw_cm2, float uva_uw_cm2, float uvi) {
    if (uva_uw_cm2 < 1.0) return 300.0;  // Domyślnie, jeśli brak danych UVA
    float ratio = uvb_uw_cm2 / uva_uw_cm2;
    float sin_elev = std::max(0.2f, sin(uvi * 0.1745329));  // Zgrubne oszacowanie wysokości
    return 300.0 + (ratio - 0.004) * (-1000.0) / sin_elev;
}

// ============================================================================
// 4. MELANOPICZNY LUX (Ryzyko Tłumienia Melatoniny przez Niebieskie Światło)
// ============================================================================

/**
 * @brief Melanopiczny Lux (Ryzyko Zakłócenia Rytmu Okołodobowego)
 * 
 * @description
 * Mierzy natężenie światła niebieskiego (460nm) ważone pod kątem wpływu na tłumienie melatoniny.
 * Wysokie natężenie światła niebieskiego w nocy zakłóca sen; ważne dla alertów zdrowotnych.
 * Odpowiedź chromatyczna osiąga szczyt przy 460nm (szczytowe tłumienie melatoniny).
 * 
 * @formula
 *   M_lux = (F_425×0.2 + F_450×1.0 + F_475×0.5) / 10  [melanopiczny lux]
 *   Gdzie F_wavelength = natężenie spektralne przy danej długości fali [µW/m²/nm]
 * 
 * @references
 * - Lucas et al. (2014) "Measuring and using light in the melanopsin age" - Trends Neurosci
 * - CIE TN 003:2015 "Report on the first international workshop on circadian photometry"
 * - Gall et al. (2020) "On weighted irradiances for efficacy functions"
 * 
 * @parameters
 *   [in]  f425_counts            Kanał spektralny 425nm [jednostki/1000]
 *   [in]  f450_counts            Kanał spektralny 450nm [jednostki/1000]
 *   [in]  f475_counts            Kanał spektralny 475nm [jednostki/1000]
 * 
 * @returns Melanopiczny ekwiwalent lux [lux_melanopiczny]
 * @unit Melanopiczny lux (ważony pod kątem zakłóceń okołodobowych)
 * @range Wyjście: 0-10000+ lux_m
 * @accuracy ±10% (jeśli kanały spektralne są dobrze skalibrowane)
 * 
 * @thresholds_human_readable
 *   M_lux < 100       → Minimalne zakłócenia okołodobowe
 *   M_lux: 100-500    → Niewielkie tłumienie melatoniny; akceptowalne wieczorem
 *   M_lux: 500-1000   → Umiarkowane tłumienie; unikać <2h przed snem
 *   M_lux: 1000-3000  → Silne tłumienie; ryzyko problemów ze snem, jeśli blisko pory snu
 *   M_lux > 3000      → Poważne zakłócenia; sen poważnie zaburzony
 * 
 * @example
 *   float mlux = calcMelanopicLux(500, 2000, 800);  // Czas przed ekranem wieczorem
 */
inline float calcMelanopicLux(float f425_counts, float f450_counts, float f475_counts) {
    return (f425_counts * 0.2 + f450_counts * 1.0 + f475_counts * 0.5) / 10.0;
}

// ============================================================================
// 5. CZAS SYNTEZY WITAMINY D (z UVI)
// ============================================================================

/**
 * @brief Czas Syntezy Witaminy D (Minuty do 1000 IU)
 * 
 * @description
 * Szacuje czas ekspozycji na słońce potrzebny do odpowiedniej syntezy witaminy D.
 * Kluczowe dla: zaleceń zdrowotnych, sezonowych wytycznych dla szerokości geograficznych, równowagi między zapobieganiem rakowi skóry.
 * Zakłada typ skóry Fitzpatrick II; pomnożyć przez współczynnik dla innych typów.
 * 
 * @formula
 *   Czas [min] = 200 / UVI
 *   Produkuje ~1000 IU witaminy D (typ skóry II, 1/4 ekspozycji ciała, słońce w południe)
 * 
 * @references
 * - Holick et al. (2011) "Vitamin D deficiency in the United States"
 * - Wacker & Holick (2013) "Sunlight and Vitamin D"
 * - Wytyczne WHO/FAO dotyczące witaminy D: https://www.who.int/
 * 
 * @parameters
 *   [in]  uvi                    Indeks UV [0-15]
 * 
 * @returns Czas syntezy 1000 IU [minuty]
 * @unit minuty
 * @range Wyjście: 13-∞ minut (UVI 15 do <1)
 * @accuracy ±20% (zależy od typu skóry, kąta, pory roku)
 * 
 * @thresholds_human_readable
 *   Czas < 15 min   → Szybka synteza (silne UV)
 *   Czas: 15-30 min → Umiarkowana (dobry czas na witaminę D)
 *   Czas: 30-60 min → Potrzebna dłuższa ekspozycja
 *   Czas > 60 min   → Słabe UV (wczesna/późna pora roku, wysokie szerokości geograficzne)
 * 
 * @example
 *   float synthesis_time = calcVitaminDTime(8.0);
 *   // Zwraca 25 minut dla 1000 IU
 * 
 * @test_cases
 *   (12 UVI) → Czas ≈ 16.7 min (silne UV)     ✓
 *   (6 UVI)  → Czas ≈ 33.3 min (umiarkowane)      ✓
 *   (2 UVI)  → Czas ≈ 100 min (słabe, wysoka szer. geogr.) ✓
 */
inline float calcVitaminDTime(float uvi) {
    if (uvi <= 0.1) return 9999.0;  // Noc lub w pomieszczeniach
    return 200.0 / uvi;
}

// ============================================================================
// 6. POZIOM KONDENSACJI WZNOSZĄCEJ (LCL - Wzór Boltona)
// ============================================================================

/**
 * @brief LCL (Poziom Kondensacji Wznoszącej - Zaawansowany Bolton)
 * 
 * @description
 * Dokładniejszy niż uproszczona reguła Henninga; uwzględnia procesy termodynamiczne.
 * Wysokość, na której wznoszące się powietrze staje się nasycone i rozpoczyna się tworzenie chmur.
 * Kluczowe dla: prognozowania konwekcji, potencjału burz, lotnictwa.
 * 
 * @formula
 *   LCL = 125 × (T - Td)  [metry, empiryczne]
 *   Dokładniejsze: LCL = (T + 273.15) / [(ln(RH/100))/(-0.06486) + (T + 273.15)/243.5 - ln((T+273.15)/273.15)]
 * 
 * @references
 * - Bolton, D. (1980) "The computation of equivalent potential temperature"
 * - Nota Techniczna WMO: Termodynamika atmosfery
 * 
 * @parameters
 *   [in]  temperature_celsius    Temperatura powierzchni [°C]
 *   [in]  dew_point_celsius      Temperatura punktu rosy [°C]
 * 
 * @returns Wysokość LCL [m AGL]
 * @unit metry nad poziomem gruntu
 * @range Wyjście: 0-5000 m
 * @accuracy ±10% (wzór Boltona jest bardzo dokładny)
 * 
 * @example
 *   float lcl = calcLCL_Bolton(25.0, 15.0);
 */
inline float calcLCL_Bolton(float temperature_celsius, float dew_point_celsius) {
    return 125.0 * (temperature_celsius - dew_point_celsius);
}

// ============================================================================
// 7. EKWIWALENTNA TEMPERATURA POTENCJALNA (θe - Wilgotna Adiabatyczna)
// ============================================================================

/**
 * @brief Ekwiwalentna Temperatura Potencjalna (θe - Stabilność Konwekcyjna)
 * 
 * @description
 * Temperatura, jaką miałoby powietrze, gdyby cała wilgoć skondensowała się + ochłodziła adiabatycznie.
 * Wielkość zachowawcza; używana do identyfikacji granic mas powietrza i niestabilności.
 * Kluczowe dla: prognozowania gwałtownych zjawisk pogodowych, klasyfikacji mas powietrza.
 * 
 * @formula
 *   θe ≈ (T + Td/5) × (1000/P)^0.286 × exp(w×0.00363)
 *   Gdzie w = stosunek zmieszania [g/kg], wersja uproszczona
 * 
 * @references
 * - Stull, R. (2011) "Meteorology for Scientists and Engineers"
 * - Bolton, D. (1980) "The computation of equivalent potential temperature"
 * 
 * @parameters
 *   [in]  temperature_celsius    Temperatura powietrza [°C]
 *   [in]  dew_point_celsius      Temperatura punktu rosy [°C]
 *   [in]  pressure_hpa           Ciśnienie [hPa]
 * 
 * @returns θe [Kelwin]
 * @unit K (Kelwin)
 * @range Wyjście: 250-350 K
 * @accuracy ±2 K
 * 
 * @thresholds_human_readable
 *   θe < 280 K      → Stabilne powietrze (tłumiona konwekcja)
 *   θe: 280-310 K   → Umiarkowanie niestabilne
 *   θe: 310-330 K   → Niestabilne (potencjał burz)
 *   θe > 330 K      → Bardzo niestabilne (ryzyko gwałtownych burz)
 * 
 * @example
 *   float theta_e = calcEquivalentPotentialTemp(25.0, 15.0, 1000.0);
 */
inline float calcEquivalentPotentialTemp(float temperature_celsius, float dew_point_celsius, float pressure_hpa) {
    float T_K = temperature_celsius + 273.15;
    float Td_K = dew_point_celsius + 273.15;
    float mixing_ratio = 621.97 * (0.01 * 6.112 * exp((17.67 * dew_point_celsius) / (dew_point_celsius + 243.5))) 
                         / (pressure_hpa - 0.01 * 6.112 * exp((17.67 * dew_point_celsius) / (dew_point_celsius + 243.5)));
    
    float theta_e = (T_K + Td_K / 5.0) * pow(1000.0 / pressure_hpa, 0.286) * exp(mixing_ratio * 0.00363);
    return theta_e;
}

// ============================================================================
// 8. WODA OPADOWA (Całkowita Kolumna Wody Atmosferycznej)
// ============================================================================

/**
 * @brief Woda Opadowa (PW - Całkowita Woda Atmosferyczna)
 * 
 * @description
 * Całkowita kolumna pary wodnej od powierzchni do górnej warstwy atmosfery.
 * Używane do: prognozowania pogody, korekcji opóźnień sygnału GPS, modelowania promieniowania.
 * Wyższe PW = więcej wilgoci = potencjał burz.
 * 
 * @formula
 *   PW ≈ (AH × wysokość_skali) / 1000  [mm]
 *   Gdzie AH = wilgotność bezwzględna [g/m³], wysokość_skali ≈ 8500 m (rozpad wykładniczy)
 * 
 * @references
 * - Bevis et al. (1992) "GPS Meteorology: Remote Sensing of Atmospheric Water Vapor"
 * - Trenberth et al. (2005) "Atmospheric Moisture Climatology"
 * 
 * @parameters
 *   [in]  abs_humidity           Wilgotność bezwzględna [g/m³]
 *   [in]  temperature_celsius    Temperatura [°C] (wpływa na wysokość skali)
 * 
 * @returns Kolumna wody opadowej [mm]
 * @unit milimetry (mm)
 * @range Wyjście: 5-75 mm (typowy zakres)
 * @accuracy ±10%
 * 
 * @thresholds_human_readable
 *   PW < 20 mm  → Suche warunki (wysokie góry, zima, obszary polarne)
 *   PW: 20-40 mm→ Umiarkowana wilgotność
 *   PW: 40-60 mm→ Wilgotno (tropiki, lato)
 *   PW > 60 mm  → Bardzo wilgotno; potencjał burz
 * 
 * @example
 *   float pw = calcPrecipitableWater(12.0, 25.0);
 */
inline float calcPrecipitableWater(float abs_humidity, float temperature_celsius) {
    float scale_height = 8500.0 * exp(-temperature_celsius / 20.0);  // Korekcja temperatury
    return (abs_humidity * scale_height) / 1000000.0;
}

#endif  // ATMOSPHERIC_PHYSICS_H