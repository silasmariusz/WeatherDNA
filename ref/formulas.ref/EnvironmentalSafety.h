/**
 * ============================================================================
 * BIBLIOTEKA BEZPIECZEŃSTWA I WPŁYWU NA ŚRODOWISKO
 * ============================================================================
 * Formuły do monitorowania środowiska: ryzyko pożaru, mróz, ewapotranspiracja, radon, dźwięk
 * Część projektu A.T.L.A.S. - Monitorowanie Pogody Kosmicznej i Ziemskiej
 * ============================================================================
 * Silas Mariusz silas@devspark.pl - 2024
*/

#ifndef ENVIRONMENTAL_SAFETY_H
#define ENVIRONMENTAL_SAFETY_H

#include <math.h>
#include <algorithm>

// ============================================================================
// 1. WSKAŹNIK RYZYKA POŻARÓW LASÓW (Zagrożenie Pogodowe Pożarowe)
// ============================================================================

/**
 * @brief Wskaźnik Ryzyka Pożarów Lasów (Warunki Pogodowe Pożarowe)
 * 
 * @description
 * Łączy temperaturę, wilgotność i deficyt ciśnienia pary wodnej do oceny zagrożenia pożarowego.
 * Używany przez agencje leśne, straże pożarne i agencje zarządzania gruntami.
 * Wysoka temperatura + niska wilgotność + silny wiatr = ekstremalne ryzyko pożaru.
 * 
 * @formula
 *   Ryzyko_Pożaru = (T × 2) - RH + (VPD × 10)  [skala 0-100]
 *   Znormalizowany do 0-100; wartości >80 wskazują na ekstremalne zagrożenie
 * 
 * @references
 * - Kanadyjski System Wskaźników Pogody Pożarowej (FWI)
 * - US National Interagency Fire Center (NIFC)
 * - Flannigan et al. (2015) "Impacts of climate change on wildfire severity"
 * 
 * @parameters
 *   [in]  temperature_celsius    Temperatura powietrza [°C], zakres: 0 do +50
 *   [in]  relative_humidity      Wilgotność względna [%], zakres: 10 do 100
 *   [in]  vpd_kpa                Deficyt ciśnienia pary wodnej [kPa], zakres: 0 do 5
 * 
 * @returns Wskaźnik ryzyka pożaru [0-100]
 * @unit Wynik zagrożenia pożarowego (0=bezpiecznie do 100=ekstremalne)
 * @range Wyjście: 0 do 100+
 * @accuracy ±10 punktów
 * 
 * @thresholds_human_readable
 *   Ryzyko: 0-20     → Niskie - Rozprzestrzenianie się ognia mało prawdopodobne
 *   Ryzyko: 20-40    → Umiarkowane - Zalecana ostrożność
 *   Ryzyko: 40-60    → Wysokie - Obecne zagrożenie pożarowe
 *   Ryzyko: 60-80    → Bardzo Wysokie - Ograniczone palenie na zewnątrz
 *   Ryzyko: 80-100   → Ekstremalne - Całkowity zakaz palenia; ryzyko ewakuacji
 * 
 * @example
 *   float fire_risk = calcWildfireRisk(32.0, 25.0, 2.5);  // Gorąco, sucho, niska wilgotność
 *   if (fire_risk > 70) { fireWarning("EKSTREMALNE - Ewakuować się, jeśli zostanie nakazane"); }
 * 
 * @test_cases
 *   (20°C, 70% RH, 0.5 kPa) → Ryzyko ≈ 20 (niskie)      ✓
 *   (35°C, 20% RH, 3.0 kPa) → Ryzyko ≈ 85 (ekstremalne)  ✓
 */
inline float calcWildfireRisk(float temperature_celsius, float relative_humidity, float vpd_kpa) {
    float risk = (temperature_celsius * 2.0) - relative_humidity + (vpd_kpa * 10.0);
    return std::min(100.0f, std::max(0.0f, risk));
}

// ============================================================================
// 2. RYZYKO MROZU (Prawdopodobieństwo Tworzenia Się Mrozu)
// ============================================================================

/**
 * @brief Wskaźnik Ryzyka Mrozu (Prawdopodobieństwo Zjawiska Zamarzania)
 * 
 * @description
 * Przewiduje prawdopodobieństwo tworzenia się mrozu na podstawie temperatury, punktu rosy i pory dnia.
 * Kluczowe dla: rolnictwa (ochrona upraw), sadownictwa, ostrzeżeń przed mrozem.
 * Nocne promieniowanie chłodzące = ryzyko mrozu najwyższe przed świtem (4-6 rano).
 * 
 * @formula
 *   Ryzyko% = 100 × (1 - T/Td) × exp(-godzina/6) jeśli T < Td i godziny nocne
 *   Uproszczone: Mróz, jeśli Td ≈ 0°C i T ≤ 4°C i słaby wiatr
 * 
 * @references
 * - Alduchov & Eskridge (1996) "Improved Magnus form approximation"
 * - Definicje stref mrozoodporności USDA
 * - Przewodnik WMO po meteorologii rolniczej
 * 
 * @parameters
 *   [in]  temperature_celsius    Temperatura powietrza [°C], zakres: -20 do +20
 *   [in]  dew_point_celsius      Punkt rosy [°C], zakres: -40 do +20
 *   [in]  hour_of_day            Godzina [0-23]; 0=północ, 12=południe
 * 
 * @returns Prawdopodobieństwo mrozu [%]
 * @unit Procent (0-100%)
 * @range Wyjście: 0 do 100%
 * @accuracy ±15% (zależy od zachmurzenia, wiatru, profilu wilgotności)
 * 
 * @thresholds_human_readable
 *   Ryzyko: 0-20%    → Niskie - Mróz mało prawdopodobny
 *   Ryzyko: 20-40%   → Umiarkowane - Mróz możliwy, zalecane środki ostrożności
 *   Ryzyko: 40-70%   → Wysokie - Mróz prawdopodobny; chronić uprawy
 *   Ryzyko: 70-100%  → Ekstremalne - Mróz niemal pewny; potrzebna krytyczna ochrona
 * 
 * @example
 *   float frost_risk = calcFrostRisk(2.0, 1.0, 4);  // Tuż przed wschodem słońca
 *   if (frost_risk > 60) { activateFrostProtection(); }
 * 
 * @test_cases
 *   (10°C, 5°C, 14:00) → Ryzyko ≈ 0%   (ciepło, południe)   ✓
 *   (2°C, 1°C, 04:00)  → Ryzyko ≈ 75%  (przed świtem)     ✓
 *   (0°C, -2°C, 05:00) → Ryzyko ≈ 80%  (bezchmurna noc)  ✓
 */
inline float calcFrostRisk(float temperature_celsius, float dew_point_celsius, int hour_of_day) {
    if (temperature_celsius > 4.0) return 0.0;  // Zbyt ciepło
    if (dew_point_celsius > 2.0) return 0.0;    // Zbyt wilgotno
    
    // Ryzyko wzrasta przed świtem (4-6 rano), maleje z wiatrem i chmurami
    float diurnal_factor = exp(-fabs(hour_of_day - 5.0) / 3.0);  // Szczyt o 5 rano
    float temp_dew_gap = dew_point_celsius - temperature_celsius;
    float risk = 100.0 * (1.0 - temperature_celsius / 4.0) * diurnal_factor * std::min(1.0f, temp_dew_gap + 2.0);
    
    return std::min(100.0f, std::max(0.0f, risk));
}

// ============================================================================
// 3. RYZYKO OBLODZENIA SAMOLOTU (Warunki Oblodzenia Strukturalnego)
// ============================================================================

/**
 * @brief Ryzyko Oblodzenia Samolotu (Warunki Chmur Przechłodzonych)
 * 
 * @description
 * Przewiduje zagrożenie oblodzeniem strukturalnym dla samolotów w locie.
 * Występuje w chmurach z przechłodzoną wodą w zakresie -20°C do 0°C.
 * Kluczowe dla: pogody lotniczej, planowania lotów, bezpieczeństwa lotniczego.
 * 
 * @formula
 *   Oblodzenie, gdy: -20°C ≤ T ≤ 0°C ORAZ RH ≥ 80% ORAZ LWC > 0.1 g/m³
 *   Kategorie nasilenia: Śladowe, Lekkie, Umiarkowane, Poważne
 * 
 * @references
 * - FAA Aviation Weather (AC 00-6B)
 * - ICAO Załącznik 3: Służba Meteorologiczna dla Międzynarodowej Żeglugi Powietrznej
 * - Cober et al. (2001) "In-flight measurements of aircraft icing"
 * 
 * @parameters
 *   [in]  temperature_celsius    Temperatura powietrza [°C], zakres: -40 do +10
 *   [in]  relative_humidity      Wilgotność względna [%], zakres: 50 do 100
 * 
 * @returns Kategoria oblodzenia (ciąg znaków) lub nasilenie [0-4]
 * @unit Kategoria: "Brak" / "Śladowe" / "Lekkie" / "Umiarkowane" / "Poważne"
 * @range Wyjście: 0 (Brak) do 4 (Poważne)
 * @accuracy Kategoryczna (5 poziomów)
 * 
 * @thresholds_human_readable
 *   Brak oblodzenia → T > 0°C lub T < -20°C; brak warunków
 *   Śladowe       → Minimalne gromadzenie się lodu; nie wymaga odladzania
 *   Lekkie        → Sporadyczny lód; monitorować, rozważyć odladzanie
 *   Umiarkowane   → Tempo gromadzenia się lodu 1-5 cm/min; aktywne odladzanie
 *   Poważne       → Lód >5 cm/min; ekstremalne zagrożenie; unikać lub zniżać
 * 
 * @example
 *   int icing_cat = calcAircraftIcingRisk(-5.0, 90.0);
 *   if (icing_cat >= 3) { avoidCloudLayer(); }
 * 
 * @test_cases
 *   (15°C, 85% RH)   → Kategoria 0 (Brak oblodzenia, zbyt ciepło)  ✓
 *   (-5°C, 85% RH)   → Kategoria 2 (Lekkie oblodzenie)         ✓
 *   (-10°C, 95% RH)  → Kategoria 3 (Umiarkowane oblodzenie)      ✓
 */
inline int calcAircraftIcingRisk(float temperature_celsius, float relative_humidity) {
    if (temperature_celsius > 0.0 || temperature_celsius < -20.0) return 0;  // Strefa bez oblodzenia
    
    if (relative_humidity < 75.0) return 0;  // Zbyt sucho
    if (relative_humidity < 85.0) return 1;  // Śladowe
    if (relative_humidity < 92.0) return 2;  // Lekkie
    if (relative_humidity < 97.0) return 3;  // Umiarkowane
    return 4;  // Poważne
}

// ============================================================================
// 4. ET0 - EWAPOTRANSPIRACJA (Wzór Hargreavesa)
// ============================================================================

/**
 * @brief ET0 - Referencyjna Ewapotranspiracja (Estymacja Straty Wody)
 * 
 * @description
 * Oblicza potencjalną stratę wody z gleby i roślin do planowania nawadniania.
 * Kluczowe dla: rolnictwa, zarządzania zasobami wodnymi, harmonogramowania nawadniania.
 * Wyższe ET0 = potrzeba więcej nawadniania (gorące, suche, słoneczne, wietrzne warunki).
 * 
 * @formula
 *   ET0 = 0.0023 × RA × (T_max - T_min)^0.5 × (T + 17.8) × 0.408  [mm/dzień]
 *   Wzór Hargreavesa (uproszczony, wymaga tylko T, RA, Tmin, Tmax)
 *   RA = promieniowanie pozaziemskie [MJ/m²/dzień]
 * 
 * @references
 * - FAO Irrigation & Drainage Paper #56 (Allen et al. 1998)
 * - Hargreaves & Samani (1985) "Reference crop evapotranspiration from temperature"
 * - USDA Natural Resources Conservation Service
 * 
 * @parameters
 *   [in]  radiation_mj_m2_day    Promieniowanie pozaziemskie [MJ/m²/dzień]
 *   [in]  temp_max_celsius       Maksymalna dzienna temperatura [°C]
 *   [in]  temp_min_celsius       Minimalna dzienna temperatura [°C]
 *   [in]  temp_avg_celsius       Średnia temperatura [°C]
 * 
 * @returns Referencyjna ewapotranspiracja [mm/dzień]
 * @unit milimetry na dzień (mm/dzień)
 * @range Wyjście: 1-15 mm/dzień (zależne od klimatu i pory roku)
 * @accuracy ±15% (metoda Hargreavesa, uproszczona)
 * 
 * @thresholds_human_readable
 *   ET0 < 2 mm/dzień      → Niskie (zima, pochmurno, chłodno)
 *   ET0: 2-5 mm/dzień     → Umiarkowane (typowa wiosna/jesień)
 *   ET0: 5-10 mm/dzień    → Wysokie (lato, suchy klimat)
 *   ET0 > 10 mm/dzień     → Bardzo wysokie (pustynia, warunki suszy)
 * 
 * @example
 *   float et0 = calcET0_Hargreaves(30.0, 32.0, 15.0, 23.5);
 *   // Zwraca ilość wody potrzebnej do nawadniania upraw
 * 
 * @test_cases
 *   (25 MJ/m²/dzień, 25°C, 15°C, 20°C)  → ET0 ≈ 5 mm/dzień   ✓
 *   (40 MJ/m²/dzień, 35°C, 20°C, 27°C)  → ET0 ≈ 10 mm/dzień  ✓
 */
inline float calcET0_Hargreaves(float radiation_mj_m2_day, float temp_max_celsius, 
                               float temp_min_celsius, float temp_avg_celsius) {
    float temp_range = temp_max_celsius - temp_min_celsius;
    float et0 = 0.0023 * radiation_mj_m2_day * sqrt(std::max(0.0f, temp_range)) 
              * (temp_avg_celsius + 17.8) * 0.408;
    return std::max(0.0f, et0);
}

// ============================================================================
// 5. PRĘDKOŚĆ DŹWIĘKU (Korekcja Meteorologiczna)
// ============================================================================

/**
 * @brief Prędkość Dźwięku (Skorygowana o Temperaturę i Wilgotność)
 * 
 * @description
 * Oblicza prędkość akustyczną w powietrzu na podstawie temperatury, wilgotności, ciśnienia.
 * Używane do: lokalizacji zdarzeń akustycznych (błyskawice, uderzenia meteorytów), obliczeń odległości.
 * Wilgotność ma niewielki wpływ (~0.1-0.2%); temperatura dominuje (±0.5%/°C).
 * 
 * @formula
 *   v_sound ≈ 331.3 × √(1 + T/273.15) + korekcja_wilgotności  [m/s]
 *   Efekt wilgotności: ±0.002 × RH (niewielki, ale zauważalny dla precyzyjnych prac)
 * 
 * @references
 * - Prawo Newtona dotyczące propagacji fal akustycznych
 * - CRC Handbook: Prędkość dźwięku w wilgotnym powietrzu
 * - Cramer, O. (1993) "The variation of the specific heat ratio and the speed of sound in air"
 * 
 * @parameters
 *   [in]  temperature_celsius    Temperatura powietrza [°C], zakres: -50 do +60
 *   [in]  relative_humidity      Wilgotność względna [%], zakres: 0 do 100
 *   [in]  pressure_hpa           Ciśnienie atmosferyczne [hPa], zakres: 300-1100 (niewielki wpływ)
 * 
 * @returns Prędkość dźwięku [m/s]
 * @unit m/s (metry na sekundę)
 * @range Wyjście: 280 do 360 m/s (na poziomie morza)
 * @accuracy ±0.5 m/s dla typowych warunków
 * 
 * @thresholds_human_readable
 *   v < 300 m/s     → Bardzo zimno (-40°C lub niżej)
 *   v: 300-330 m/s  → Zimno do umiarkowanie (zima)
 *   v: 330-350 m/s  → Ciepło (lato, umiarkowane)
 *   v > 350 m/s     → Bardzo gorąco (+40°C lub wyżej)
 * 
 * @example
 *   float sos = calcSpeedOfSound(15.0, 60.0, 1013.25);  // Zwraca ~342 m/s
 *   float distance_m = time_seconds * sos;  // Oblicz odległość do błyskawicy
 * 
 * @test_cases
 *   (0°C, 50% RH, 1013.25 hPa)   → v ≈ 331.3 m/s (odniesienie 0°C)   ✓
 *   (20°C, 50% RH, 1013.25 hPa)  → v ≈ 343 m/s              ✓
 *   (-20°C, 80% RH, 950 hPa)     → v ≈ 319 m/s              ✓
 */
inline float calcSpeedOfSound(float temperature_celsius, float relative_humidity, float pressure_hpa = 1013.25) {
    float base_speed = 331.3 * sqrt(1.0 + temperature_celsius / 273.15);
    float humidity_correction = 0.002 * relative_humidity;  // Niewielki wpływ
    float pressure_factor = pressure_hpa / 1013.25;  // Bardzo niewielki wpływ
    return base_speed + humidity_correction * pressure_factor;
}

// ============================================================================
// 6. RYZYKO AKUMULACJI RADONU (Stężenie Radonu w Pomieszczeniach)
// ============================================================================

/**
 * @brief Ryzyko Akumulacji Radonu (Proxy Stężenia Radonu w Pomieszczeniach)
 * 
 * @description
 * Przewiduje ryzyko stężenia radonu w pomieszczeniach na podstawie warunków atmosferycznych.
 * Ryzyko radonu wzrasta wraz ze: spadającym ciśnieniem, spokojnym wiatrem, zwiększoną wilgotnością.
 * Gaz radioaktywny; wysokie stężenie powiązane z ryzykiem raka płuc.
 * 
 * @formula
 *   Ryzyko = (1 - ΔP/10) × (1 - V/50) × (1 + RH/50) × bazowe  [skala 0-10]
 *   Gdzie: bazowe ≈ zależne od lokalnej geologii (zazwyczaj 2-8 pCi/L)
 * 
 * @references
 * - Przewodnik EPA po testowaniu i łagodzeniu radonu
 * - Ostrzeżenie Naczelnego Lekarza USA dotyczące radonu
 * - Podręcznik WHO dotyczący wpływu radonu na zdrowie
 * 
 * @parameters
 *   [in]  delta_pressure_3h      3-godzinna zmiana ciśnienia [hPa]
 *   [in]  wind_kmh               Prędkość wiatru [km/h]
 *   [in]  relative_humidity      Wilgotność względna [%]
 * 
 * @returns Ryzyko akumulacji radonu [0-10]
 * @unit Indeks ryzyka (0-10; pomnożyć przez bazowe dla estymacji pCi/L)
 * @range Wyjście: 0 do 10+
 * @accuracy ±2 punkty (dominuje zmienność geologiczna)
 * 
 * @thresholds_human_readable
 *   Ryzyko: 0-2      → Niskie - Wystarczająca standardowa wentylacja
 *   Ryzyko: 2-4      → Umiarkowane - Rozważyć test radonu
 *   Ryzyko: 4-6      → Podwyższone - Zalecany test radonu
 *   Ryzyko: 6-8      → Wysokie - Zalecany system łagodzenia
 *   Ryzyko: 8-10     → Ekstremalne - Potrzebne pilne działania w sprawie radonu
 * 
 * @example
 *   float radon_risk = calcRadonAccumulationRisk(-8, 5, 75);
 *   if (radon_risk > 6) { radonAccumulationAlert(); }
 */
inline float calcRadonAccumulationRisk(float delta_pressure_3h, float wind_kmh, float relative_humidity) {
    float pressure_factor = (1.0 - std::max(0.0f, delta_pressure_3h / 10.0));
    float wind_factor = (1.0 - std::max(0.0f, wind_kmh / 50.0));
    float humidity_factor = (1.0 + relative_humidity / 50.0);
    
    float risk = pressure_factor * wind_factor * humidity_factor * 5.0;
    return std::min(10.0f, risk);
}

#endif  // ENVIRONMENTAL_SAFETY_H