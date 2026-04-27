# Podłączanie czujników MICS-4514 i CJMCU-6814 (Logika 5V)

## Zagrożenie architektoniczne
Czujniki z serii MICS (w tym moduły CJMCU-6814 i MICS-4514) to układy analogowe MOx (Metal-Oxide), które do prawidłowego rozgrzania grzałki pomiarowej bezwzględnie wymagają zasilania **5V**.

Używana w tym projekcie płyta **ESP32-S3 DevKitC-1** pracuje wyłącznie w logice **3.3V** (zgodnie ze Złotymi Zasadami zdefiniowanymi w pliku `SKILL.md`). Wszelkie wstrzyknięcie sygnału 5V bezpośrednio na wejścia ADC mikroprocesora spowoduje trwałe, fizyczne spalenie krzemu i zwarcie całego kontrolera do masy.

## Poprawne podłączenie (Wymóg Level Shiftera)
Potwierdzam, że **MOŻNA** bezpiecznie podłączyć te czujniki do ESP32-S3 pod jednym rygorystycznym warunkiem:
Wymagane jest użycie dedykowanego, dwukierunkowego konwertera poziomów logicznych (Level Shifter Bi-Directional, np. oparty na tranzystorach MOSFET typu BSS138), bądź analogowego dzielnika napięcia dla sygnałów ADC.

### Schemat połączeń

**1. Zasilanie (Heater Power)**
* Czujnik `VCC` / `5V` --> Płyta ESP32 `5V` (VBUS / VIN pin)
* Czujnik `GND` --> Płyta ESP32 `GND`

**2. Linie Pomiarowe (Przez Level Shifter / Dzielnik dla ADC)**
* Czujnik `PRE` / `NO2` / `CO` / `NH3` (5V Logic) --> `HV1`, `HV2`, `HV3` (Wysokie napięcie na Level Shifterze)
* Level Shifter `LV1`, `LV2`, `LV3` (3.3V Logic) --> Piny wejść analogowych na ESP32-S3 (np. GPIO 6, 7, 8, skonfigurowane jako `ADC1`).

*Ważna uwaga ADC:* Moduły CJMCU wypuszczają analogowe napięcie proporcjonalne do oporności gazu. Ponieważ konwertery I2C (typu BSS138) są zaprojektowane do sygnałów cyfrowych, przy przesyłaniu bardzo delikatnych sygnałów analogowych najdokładniejszą i najprostszą metodą jest zastosowanie twardego **dzielnika napięcia na rezystorach (np. R1=10kΩ, R2=20kΩ)** dla każdej linii ADC opuszczającej czujnik MICS, co fizycznie zbije sygnał 0-5V do skali 0-3.3V tolerowanej przez ESP32.

### Implementacja obsługi
Poniżej znajduje się zalążek kodu do bezpiecznej obsługi w stacji A.T.L.A.S.:
Wymagane będzie podłączenie zewnętrznego przetwornika ADC po szynie I2C (np. ADS1115), ponieważ ESP32 cierpi na nieliniowość wewnętrznego przetwornika i zniekształciłoby to odczyty.

```cpp
// Przykładowy pseudo-kod integracji ADS1115 (3.3V I2C MUX) do czytania 5V MICS
// ADS1115 toleruje czytanie do +0.3V ponad swoje VDD, więc z dzielnikiem napięcia to bezpieczne.
#include <Adafruit_ADS1X15.h>
Adafruit_ADS1115 ads; // 16-bitowy ADC

void initMICS() {
    if (tcaselect(CH_ADC_MICS)) {
        if (ads.begin()) {
            ads.setGain(GAIN_ONE); // Zakres pomiarowy do 4.096V
            Serial.println("  |- MICS-4514 (przez ADS1115): ONLINE");
        }
    }
}
```
Zostanie to wdrożone w następnej fazie podpinania nowych urządzeń na MUX.
