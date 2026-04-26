## 1. Architektura Systemu (Topologia I2C)

# SKILL: WeatherDNA Project

## 1. Hardware Configuration
*   **Board**: ESP32-S3 DevKitC-1 (https://www.espboards.dev/esp32/esp32-s3-devkitc-1/)
*   **Memory**: 16MB FLASH / 8MB PSRAM
*   **Bus Config**:
    *   **I2C SDA**: GPIO 4 (D3)
    *   **I2C SCL**: GPIO 5 (D4)
    *   **UART TX**: GPIO 43 (D6)
    *   **UART RX**: GPIO 44 (D7)
    *   **RGB LED**: GPIO 48 (Status Indicator)

## 2. Stevenson Screen Topology (MUX 0x72)
*   **CH0** | BME688 AI (0x77)
*   **CH1** | ZMOD4510 (0x33)
*   **CH2** | SCD41 (0x62) + SGP41 (0x59)
*   **CH3** | BMV080 (0x57)
*   **CH4** | BME690 (0x77)
*   **CH5** | SHT45 (0x44)
*   **CH6** | ILPS22QS (0x5C)
*   **CH7** | BMP585 (0x46)

---

@@ WYMAGANE

### 🔌 Standardy Operacyjne
* **Standardowy adres IP (Statyczny):** 10.100.200.18.
* **Protokół Po-Zadaniowy:** Zawsze po pełnym zadaniu wykonaj **OTA Update** (pio run -t upload --upload-port 10.100.200.18). Jeśli urządzenie nie odpowiada na ping, zignoruj.

---

## 2. Prawa Architektury (Złote Zasady Inżynieryjne)

### Zasada Napięcia (Tolerancja Krzemu)
1.  **ESP32-S3 DevKitC-1 16MB FLASH 8MB PSRAM  wyłącznie w logice 3.3V**. Wstrzyknięcie 5V na linie SDA/SCL, przerwania (IRQ) lub zasilanie modułów (VCC) bez wbudowanego stabilizatora (LDO) pali krzem i zwiera I2C do masy.

### Zasada Multipleksera (Centrala Telefoniczna)
1.  **Nigdy nie otwieraj wszystkich kanałów MUX naraz (`0xFF`)**. Każdy wpięty moduł posiada własne rezystory podciągające (pull-up, np. 10 kΩ). Otwarcie 8 kanałów łączy je równolegle, drastycznie zmniejszając opór (np. do 1.25 kΩ). Magistrala traci zdolność ściągnięcia sygnału do zera (LOW), co całkowicie blokuje komunikację.
2.  **Pojemność kabli**: Otwarcie bramki MUX dla Klatki Stevensona dodaje pojemność 30 cm kabla do magistrali. Kanał musi być otwierany tuż przed odczytem i zamykany natychmiast po nim (Izolacja Szumów).
3.  **Biblioteka BSEC (BME690)**: Jest "ślepa" na architekturę MUX. W pętli `loop()` **zawsze** trzeba wywołać `muxSelect(MUX_KLATKA, CH_BME690)` ułamek sekundy przed wykonaniem funkcji `bsec.run()`. W przeciwnym razie BSEC wyrzuci błąd `UNKNOWN_CODE_-12` (BSEC_E_COMM_FAIL).

---

## 3. Anomalie i Diagnostyka Sprzętowa (Troubleshooting)

### A. Twardy Reset Watchdoga (Zawieszenie Systemu)
* **Objaw**: Logi urywają się na inicjalizacji modułu, odliczanie czasu zacina się, po czym następuje wyrzut rdzenia: `Task watchdog got triggered -> abort() was called`.
* **Przyczyna**: Latch-Up magistrali I2C (Zatrzaśnięcie). Krzem zwarty jest do masy przez fizyczne uszkodzenie (przebicie), błąd adresacji, lub szum elektryczny. Kod wchodzi w nieskończoną pętlę `while(Wire.available() == 0)`.
* **Rozwiązanie (Triage)**:
    1. Odpiąć fizycznie wszystkie kable z danego MUXa.
    2. Sprawdzić "goły" MUX skanerem.
    3. Podpinać czujniki po jednym i restartować. Czujnik wywołujący Watchdog jest zwarty/uszkodzony.

### B. Syndrom "Last Man Standing" (Socjopatia układu AS3935)
* **Objaw**: Stacja inicjuje sprzęt. Log urywa się po radośnie wyglądającym komunikacie: 
`[HW INIT] CH5: AS3935 Lightning -> LISTENING FOR RF SPIKES` 

Po 3 minutach system wywala Watchdog.
* **Przyczyna**: To nie AS3935 działa świetnie, a zawiesza się coś innego. Jest odwrotnie! Układ DFRobot AS3935 zderza się z błędem czasu operacji (`Error 263` - `I2C_ERR_TIMEOUT`), zatrzaskuje magistralę i puszcza kod dalej. Następny w kolejce sensor (np. SCD41) próbuje użyć tej zablokowanej magistrali i to jego biblioteka łapie zawał z powodu nieskończonej pętli.
* **Rozwiązanie**: Układy AS3935 są wysoce problematyczne po I2C i podatne na Latch-up. Wymagają idealnego zasilania, bezkolizyjnego przerwania sprzętowego, lub przeniesienia na odseparowaną magistralę SPI.

### D. Błędy Sterownika I2C (Wire.cpp)
* `Error -1`: Błąd uogólniony (Unknown Error) / Latch-up na pinach. Urządzenie nie odpowiedziało, bramka logiczna zamknięta.
* `Error 263`: Timeout. Procesor wysłał zegar, ale nie dostał odpowiedzi w określonym czasie (linia zablokowana przez obcą pojemność lub inny zatrzaśnięty czujnik).
* `Read failed (Took: 241 ms)`: Najczęściej błędny routing w kodzie (`muxSelect` nie został przełączony przed odczytem). Moduł wywołany pod nieobecność wywala timeout po ok. 250 milisekundach.

---

## 4. Wzorce Programistyczne i Zasady Kodu (Software Patterns)

Architektura oparta na trzech multiplekserach TCA9548A wymaga rygorystycznego podejścia do pisania kodu w pliku `main.cpp`. Złamanie poniższych reguł skutkuje błędami `Error -12`, `Error 263` i restartami Watchdoga.

### 4.1. Złota Zasada "MUX Wrapper" (Zawsze pukaj do drzwi)
**Żadne** urządzenie znajdujące się za multiplekserem nie może być odpytywane bezpośrednio.
* **ZŁA PRAKTYKA:** Bezpośrednie wywołanie inicjalizacji lub odczytu (np. `sht4.begin()`, `bmv.read()`).
* **DOBRA PRAKTYKA:** Każda operacja I2C na urządzeniach w Klatce Stevensona lub Optyce MUSI być poprzedzona komendą przełączenia kanału i minimalnym opóźnieniem na ustabilizowanie pojemności kabla:
  ```cpp
  muxSelect(MUX_KLATKA, CH_SHT45);
  delay(5); // Czas na ustabilizowanie napięcia na linii
  sht4.getEvent(&humidity, &temp);```

### 4.2. Hermetyzacja Pętli AI (BSEC2 Rule)
Biblioteka Boscha (BSEC2) dla BME690 działa asynchronicznie w tle i nie ma pojęcia o istnieniu multipleksera.
* Funkcja `bsec.run()` musi mieć zagwarantowany fizyczny dostęp do czujnika w momencie jej wywołania.
* **Reguła Pętli:** Wewnątrz `loopTask` komenda `muxSelect(MUX_KLATKA, CH_BME690)` musi znajdować się *bezpośrednio* przed `if (bsec.run())`. Jeśli wstawisz inny kod I2C pomiędzy te dwie linijki, BSEC zwróci błąd krytyczny łączności.

### 4.3. Niezmienność Adresów (Const Type Safety)
Ze względu na mnogość zmiennych `CH_...` (kanałów) i `MUX_...` (adresów), absolutnie zakazane jest używanie zwykłych zmiennych (np. `uint8_t CH_BME690 = 0;`).
* **Reguła Typowania:** Wszystkie adresy i kanały muszą być deklarowane jako `const uint8_t`. Zapobiega to przypadkowemu nadpisaniu numeru kanału w locie (np. przez pomyłkowe użycie `=` zamiast `==` w instrukcjach warunkowych), co zrujnowałoby routing całej stacji.

### 4.4. Software Auto-Healing (Obsługa Latch-Up)
Jeśli sprzęt wyrzuci błąd Timeout (`Error 263`), oznacza to, że I2C sprzętowo się zawiesiło (Latch-up). Kod stacji musi umieć odzyskać magistralę bez użycia twardego restartu (Watchdoga):
1. Wykrycie narastającej liczby błędów (np. fail_count > 3).
2. Reset programowy magistrali: `Wire.end();` -> opóźnienie -> `Wire.begin();`.
3. **Krytyczny Krok:** Po resecie I2C, multipleksery TCA9548A resetują swój stan do ZAMKNIĘTYCH bram. Protokół Auto-Healingu musi zainicjować ponownie nie tylko same czujniki, ale również zadbać o ponowne wywołania `muxSelect` podczas ich podnoszenia.

---

## 5. Uniwersalne Standardy Kodowania (Czerpane z arduino-skills)

Każda osoba rozwijająca kod stacji A.T.L.A.S. musi bezwzględnie przestrzegać podstawowych, inżynieryjnych standardów dla systemów wbudowanych. 

### 5.1. Reguła nr 1: Bezwzględny brak blokowania (Avoid delay)
* **ZAKAZ:** Zabrania się stosowania funkcji `delay()` w głównej pętli `loopTask` lub w funkcjach obsługi czujników. Każde wywołanie `delay()` blokuje procesor, zamraża bibliotekę BSEC2 i ostatecznie doprowadza do twardego restartu z powodu sprzętowego Watchdoga (WDT).
* **ROZWIĄZANIE:** Cały kod musi opierać się na architekturze asynchronicznej (non-blocking). Do zarządzania czasem używaj wyłącznie funkcji `millis()`, timerów RTOS lub maszyn stanów (State Machine). Do precyzyjnych i krótkich przerw inicjalizacyjnych na magistrali I2C dozwolone są jedynie mikrosekundowe, bezpieczne opóźnienia.

### 5.2. Centralizacja Logów i Debugowania (ATLAS_LOG Protocol)
Stacja A.T.L.A.S. pracuje w terenie jako urządzenie typu "headless" (bez fizycznego dostępu do portu Serial/USB). Aby zapewnić możliwość przeprowadzenia analizy post-mortem (np. po twardym resecie Watchdoga), obowiązuje rygorystyczna polityka logowania.

* **ZAKAZ:** Kategoryczny zakaz używania surowych wywołań `Serial.print()`, `Serial.println()` czy `printf()` w jakimkolwiek nowym kodzie, bibliotece lub module.
* **DOBRA PRAKTYKA:** Cały ruch diagnostyczny (verbose, debug, błędy, statusy) MUSI przechodzić wyłącznie przez wrapper `ATLAS_LOG(const char* format, ...)`. 

**Dlaczego ATLAS_LOG jest krytyczny?**
1. **Routing Wielokanałowy:** Wrapper automatycznie rozdziela logi na trzy kanały: sprzętowy `Serial` (na warsztacie), ulotny `logBuffer` w RAM (dla WebUI) oraz – co najważniejsze – zewnętrzny `SYSLOG`.
2. **Czarne Skrzynki (SYSLOG):** Ulotny bufor w pamięci RAM znika bezpowrotnie przy każdym zaniku zasilania lub resecie Watchdoga. Aktywny serwer SYSLOG to jedyne miejsce, w którym można przeanalizować ułamki sekund przed katastrofą (tzw. Death Gasp). Bez SYSLOG-a stacja jest diagnostycznie "ślepa".
3. **Zarządzanie Pamięcią:** Buforowanie logów w RAM (`MAX_LOG_LINES`) musi być ściśle kontrolowane, aby zapobiec fragmentacji sterty (Heap Fragmentation). Wypychanie logów do własnego kontrolera (np. zewnętrznego brokera) zdejmuje ciężar retencji danych z małej pamięci ESP32.

### 5.3. Reguła nr 2: Weryfikowalne wyjście (Verifiable Output)
Kod stacji zawsze musi jasno raportować swój stan, informować o przebiegu operacji i pozwalać na łatwe zdiagnozowanie ewentualnych błędów.
* Zawsze używaj sformatowanych logów przez makra typu `ATLAS_LOG(...)`. 
* Każda inicjalizacja sprzętu (`init...()`) musi zwracać logiczny feedback (`ONLINE` w przypadku sukcesu lub kod błędu/`FAILED` w przypadku porażki). Kod nie ma prawa zamilknąć, gdy obwód ulega awarii.

### 5.4. Reguła nr 3: Zakaz "magicznych liczb" (No Hardcoded Pins/Addresses)
Konfiguracja sprzętowa często ewoluuje (zmieniają się podpięcia kabli do MUX).
* **ZAKAZ:** Zabrania się wpisywania "na sztywno" numerów pinów GPIO, kanałów MUX czy adresów I2C głęboko w ciele funkcji. 
* **ROZWIĄZANIE:** Każda wartość sprzętowa musi być zdefiniowana w jednym, centralnym miejscu jako `const uint8_t` (najlepiej w sekcji konfiguracji hardware na górze pliku lub w oddzielnym `config.h`). Zmiana pinu lub portu I2C musi wymagać edycji tylko w jednej, łatwej do znalezienia linijce kodu.

### 5.5. Reguła nr 4: Bezpieczeństwo Pamięci i Zmiennych
* **Timery:** Do zliczania czasu (odstępy między zrzutami danych, uptime) używaj **wyłącznie** typu `unsigned long`. Zapobiegnie to błędnym obliczeniom przy "przekręceniu" się licznika funkcji `millis()` po 50 dniach działania serwera.
* **Ochrona tablic:** Ze względu na ograniczoną wielkość pamięci operacyjnej (RAM), konieczne jest rygorystyczne pilnowanie wielkości buforów (bounds checking) przed wpisaniem do nich danych z czujników lub payloadów MQTT. Zabezpiecza to przed najczęstszym błędem systemów wbudowanych – nadpisaniem pamięci (Buffer Overflow) skutkującym wyrzuceniem stosu błędu i twardym resetem (`CORRUPTED`).

---

## 6. Obsługa Zamkniętego SDK: Sensor PM BMV080 (Black Box)

Pyłomierz laserowy BMV080 wykorzystuje zamknięte oprogramowanie (Precompiled SDK od Bosch Sensortec). Ze względu na brak dostępu do kodu źródłowego biblioteki, obowiązują rygorystyczne zasady integracji tego czujnika z resztą systemu A.T.L.A.S.

### 6.1. Zarządzanie Stanem i Błędami (Status Codes)
Wszelkie funkcje wywoływane z SDK BMV080 zwracają kody statusu (zazwyczaj typu `int32_t`). Kategorycznie zabrania się ignorowania tych kodów.
* Każdy odczyt lub inicjalizacja musi być walidowana. Używaj dedykowanego logowania, aby przechwycić błąd SDK, np.:
  `if (bmv080_status != BMV080_OK) ATLAS_LOG("[BMV080] ERROR: Kod błędu SDK: %d", bmv080_status);`
* Zrzut logu błędu jest jedyną drogą do zdiagnozowania problemu w zamkniętej bibliotece (np. uszkodzenie I2C vs. wewnętrzny błąd lasera).

### 6.2. Clock Stretching (Rozciąganie Zegara I2C) - ZAGROŻENIE TIMEOUTEM
Krzem Boscha (BMV080 oraz BME690) wykorzystuje sprzętowy *Clock Stretching*. Kiedy układ wykonuje ciężkie obliczenia na wbudowanym koprocesorze (np. klasyfikacja wielkości cząsteczek PM), fizycznie ściąga linię SCL do zera, prosząc procesor ESP32 o "chwilę cierpliwości".
* **Problem:** Domyślny limit czasu oczekiwania w bibliotece `Wire.cpp` na ESP32 to często tylko 50ms. Złożone algorytmy Boscha potrafią rozciągnąć zegar nawet do 100-150ms. Po przekroczeniu 50ms, ESP32 rzuci błędem `Error 263` (Timeout) i zerwie połączenie.
* **Rozwiązanie:** W funkcji `setup()` natychmiast po `Wire.begin()`, **MUSISZ** wydłużyć czas oczekiwania magistrali:
  ```cpp
  Wire.setTimeOut(200); // Wydłużenie timeoutu I2C do 200ms dla krzemu Boscha```

### 6.3. Cykl Życia Lasera i Tryby Pracy (Power Modes)
BMV080 używa wbudowanej diody laserowej do liczenia cząstek. Dioda ta ma skończoną żywotność. Bezmyślne trzymanie czujnika w trybie ciągłym drastycznie skróci jego bezawaryjny czas działania. Zależnie od tego, jak operuje stacja, musisz wymusić konkretny tryb pracy poprzez API:

1. **Tryb Uśpienia (Sleep Mode / Idle):** Laser i wentylator (jeśli symulowany termicznie) są wyłączone. Pobór prądu spada do minimum (mikroampery).
   * **Wdrożenie:** Kiedy stacja kończy zrzut danych (Data Dump) i przechodzi do funkcji `delay` lub `Deep Sleep`, **bezwzględnie** wywołaj funkcję SDK zatrzymującą pomiar, np.: `bmv080_stop_measurement()`.
2. **Tryb Pomiaru Ciągłego (Continuous Measurement):** Laser jest aktywny, zbierane są próbki PM do bufora.
   * **Wdrożenie:** Na minimum 30-60 sekund przed zrzutem danych obudź czujnik poleceniem np. `bmv080_start_measurement()`. Pyłomierz laserowy potrzebuje czasu na tzw. "rozpędzenie powietrza" (nawet w technologii bezwiatrakowej) i ustabilizowanie odczytu. Złotą zasadą jest: budź laser, poczekaj minutę (wykonując inne zadania systemu), odczytaj ustabilizowane dane, uśpij laser.

### 6.4. Hardware Wrappers (Wskaźniki Funkcji Odczytu/Zapisu)
Biblioteka C od Boscha wymaga podania jej tzw. wskaźników do funkcji (Function Pointers), które tłumaczą jej, jak w systemie A.T.L.A.S. działa I2C.
* **Krytyczna Architektura:** Aby wyeliminować ryzyko Latch-Upa na multiplekserze, funkcje, które przekazujesz do struktury czujnika (np. `user_i2c_read` i `user_i2c_write`), powinny same w sobie wywoływać otwarcie odpowiedniej bramki MUX-a:
  ```cpp
  int8_t user_i2c_read(uint8_t reg_addr, uint8_t *reg_data, uint32_t len, void *intf_ptr) {
      muxSelect(MUX_KLATKA, CH_BMV080); // Wymuszenie fizycznej ścieżki
      // ... reszta standardowego kodu odczytu Wire.read() ...
  }
  ```
Dzięki temu SDK Boscha staje się całkowicie odporne na to, co robi reszta programu – samo "otworzy sobie drzwi" ułamek sekundy przed każdą wewnętrzną mikrotransakcją.

### 6.5. Algorytm "Obstruction Detection" (Blokada Optyczna)
Od wersji SDK v11.2.0, algorytm posiada krytyczną funkcję wykrywania blokady optycznej (Obstruction Notification). 
* Ponieważ BMV080 jest pyłomierzem pozbawionym ruchomych części (fanless), jego działanie opiera się na idealnej czystości kanału optycznego. 
* Jeśli stacja wyłapie flagę `Obstruction detected` ze strumienia danych BMV080 (odczytywaną poprzez zmienne błędu/statusu z SDK), system A.T.L.A.S. musi oflagować te dane w payloadzie MQTT jako `OOR` (Out of Range / Anomalous) i zgłosić alert konserwacyjny. Wymaga to fizycznego przedmuchania optyki czujnika w Klatce Stevensona sprężonym powietrzem.

---

SUROWO WYMAGANE DO OBSŁUGI BOSCH BME688 i 690 używać wyłącznie pełnej biblioteki `d:\Arduino\libraries\bsec_v3-3-0-0\`


### 7. Obsługa AI Boscha: Biblioteka BSEC3 (BME690/BME688)

Oprogramowanie BSEC (Bosch Software Environmental Cluster) nie jest zwykłym sterownikiem sprzętowym. To zaawansowana maszyna stanów i algorytm predykcyjny (C-library), który przetwarza surowe odczyty (rezystancję gazu, temperaturę, wilgotność) na znormalizowane indeksy (IAQ, eCO2, bVOC). 

### 7.1. Architektura "Zarządcy" (Hardware Agnostic Flow)
Biblioteka BSEC **nie komunikuje się z magistralą I2C**. Wymaga od głównego kodu A.T.L.A.S. pełnienia roli pośrednika. Zgodnie z oficjalną dokumentacją Boscha, cykl życia pojedynczego pomiaru wygląda następująco:

1. **Planowanie:** Wywołanie `bsec_sensor_control()`. Zwraca ono strukturę `bsec_bme_settings_t`, mówiącą jak skonfigurować mikrokontroler i płytkę grzewczą (heater) BME690.
2. **Przełączenie MUX:** W tym momencie system A.T.L.A.S. musi otworzyć fizyczną drogę do czujnika: `muxSelect(MUX_KLATKA, CH_BME690);`
3. **Akcja Sprzętowa (Forced Mode):** Na podstawie ustawień z BSEC, za pomocą sprzętowego sterownika (np. BME68x API), kod wymusza na czujniku pomiar i odczytuje surowe dane.
4. **Przetwarzanie (Matematyka):** Pobrane surowe sygnały są przekazywane do `bsec_do_steps()`, gdzie wewnętrzne AI przelicza je na finalne parametry IAQ.

### 7.2. Rygor Czasowy (`next_call`)
BSEC narzuca systemowi żelazny rygor czasowy. Struktura `bsec_bme_settings_t` zawiera zmienną `next_call` (timestamp w nanosekundach), która mówi procesorowi, w której dokładnie milisekundzie ma nastąpić kolejny pomiar.
* **ZAKAZ:** Zabrania się wywoływania cyklu pomiarowego BSEC za wcześnie lub z dużym opóźnieniem. Jeśli kod A.T.L.A.S. zablokuje się (np. przez `delay()` lub zablokowany czujnik AS3935) i pominie okno czasowe `next_call`, algorytm BSEC rozsynchronizuje się i wypluje błąd krytyczny.
* Odpytywanie BSEC powinno stanowić zadanie o najwyższym priorytecie czasowym w pętli `loop()`.

### 7.3. Stan Pamięci i EEPROM (State Persistence)
Aby algorytm AI mógł obliczyć m.in. tło jakości powietrza (BaseLine), musi uczyć się otoczenia (często od 4 do 28 dni). 
BSEC przechowuje wyuczony stan w strukturach RAM ESP32. W przypadku restartu zasilania lub zadziałania Watchdoga, **cała ta wiedza przepada**.
* **Odczyt stanu (`bsec_get_state`):** A.T.L.A.S. musi regularnie (np. co godzinę lub przy każdej istotnej zmianie dokładności) wyciągać wewnętrzny state z biblioteki Boscha.
* **Zapis krytyczny (`EEPROM 0x50`):** Pobrany stan musi zostać natychmiast zapisany do zewnętrznej, niezależnej kości pamięci na głównej szynie I2C. Jest to powód, dla którego `EEPROM` nie może znajdować się za multiplekserem – dostęp do zrzutu ratunkowego musi być natychmiastowy i bezbłędny.
* **Inicjalizacja (`bsec_set_state`):** Podczas operacji `[HW INIT]` na początku pracy stacji, kod musi pobrać stan z EEPROMu i nakarmić nim BSEC przed uruchomieniem algorytmów. 

### 7.4. Konfiguracja Linkera (.a / .a)
Ponieważ BSEC to zamknięty kod binarny (Precompiled Static Library), wymaga specyficznej integracji na poziomie kompilatora.
* Upewnij się, że plik `libalgobsec.a` jest poprawnie dodany do środowiska kompilacji (PlatformIO / Arduino IDE), w przeciwnym razie przy budowaniu projektu otrzymasz błędy typu `undefined reference to bsec_init`.

---

## 8. Obsługa Sensora Renesas ZMOD4510 (Ozon / NO2)

Czujnik ZMOD4510 to układ typu MOx (Metal Oxide), który do wyliczania stężenia gazów wykorzystuje oficjalne, prekompilowane algorytmy (Firmware NO2_O3). Biblioteka ta jest całkowicie odcięta od sprzętu (Hardware Agnostic) i korzysta z warstwy abstrakcji (HAL).

### 8.1. Architektura HAL (Wskaźniki I2C i MUX)
Biblioteka Renesas do inicjalizacji wymaga wypełnienia struktury `zmod4xxx_dev_t` wskaźnikami do Twoich lokalnych funkcji sprzętowych: `read`, `write` oraz `delay`.
* **Zasada Krytyczna:** Ponieważ ZMOD4510 znajduje się w Klatce Stevensona za multiplekserem, funkcje, które przekazujesz bibliotece, **MUSZĄ** samodzielnie wymuszać otwarcie bramki MUX-a przed każdą transakcją na magistrali.

**Wzorzec implementacji (Hardware Wrapper):**
```cpp
int8_t user_zmod_i2c_read(uint8_t addr, uint8_t reg_addr, uint8_t *data_buf, uint8_t len) {
    // 1. Tarcza anty-Latch-Up: Wymuszenie fizycznej ścieżki
    muxSelect(MUX_KLATKA, CH_ZMOD4510); 
    
    // 2. Właściwy odczyt z Wire.read()
    Wire.beginTransmission(addr);
    Wire.write(reg_addr);
    // ... reszta standardowej komunikacji I2C
}```

Jeśli pominiesz przełączanie MUX-a w tych wskaźnikach, głęboko zaszyta funkcja inicjująca (np. `zmod4xxx_read_sensor_info())` uderzy w próżnię na adresie 0x33, co doprowadzi do błędu zawieszenia I2C (Error -1).

### 8.2. Zarządzanie Grzałką (Heater Profiling)
ZMOD4510 rozróżnia Ozon od Dwutlenku Azotu na podstawie zmian rezystancji polimeru pod wpływem **błyskawicznych zmian temperatury wbudowanej mikro-grzałki** (tzw. profilowanie termiczne).
* Funkcja opóźnienia (`delay`), którą przekazujesz do biblioteki HAL, jest używana przez algorytm do odliczania czasu między skokami temperatury grzałki.
* **Zakaz blokowania systemu:** Upewnij się, że Twoja funkcja `user_zmod_delay(uint32_t ms)` pod maską korzysta z funkcji nieblokującej FreeRTOS (np. `vTaskDelay(pdMS_TO_TICKS(ms))`), a nie z twardego `delay()`. Zablokowanie całego procesora podczas pomiaru ZMOD zatrzyma odliczanie innych, krytycznych czujników (jak BSEC).

### 8.3. Cykl Pomiarowy
Cykl wyciągania danych z algorytmu Renesas musi zachować ścisłą kolejność:
1. `zmod4xxx_start_measurement()`: Inicjuje cykl podgrzewania grzałki.
2. Odczekanie czasu profilu (biblioteka robi to wywołując Twój wskaźnik delay).
3. `zmod4xxx_read_adc_result()`: Pobiera surowe wartości do bufora.
4. `calc_o3_no2()` (lub podobna funkcja z algorytmu): Przelicza surowe dane ADC na gotowe wartości PPB (Parts Per Billion).

**Uwaga kalibracyjna:** Podobnie jak BME690, czujnik MOx potrzebuje "wygrzania" na wolnym powietrzu (tzw. burn-in), zanim jego odczyty NO2 i O3 staną się referencyjne. Zazwyczaj pierwsze 24-48 godzin po podłączeniu zasilania należy traktować jako stabilizację polimeru.

---

UWAGI:
Pliki dokumentujące wszystkie biblioteki znajdują się w `lib.inc/*.md`, ```../lib.inc/*.md```
lub `d:\Arduino`. 
Biblioteki w katalogach: d:\Arduino\libraries\
Domyślne wersje w katalogach: d:\Arduino\libraries.def\ (referencyjny katalog)

## 9. Aktualizować automatycznie oprogramowanie jeśli urządzenie odpowiada na ping:
AirSense2 Static IP: 10.100.200.17
Wykonać OTA update. Jeśli brak urządzenia w sieci, zignorować czynność.

---

## UWAGA! Prawa Edycji i Narzędzi (MANDATY TECHNICZNE)

### ⚠️ Obsługa błędów narzędzia 'replace'
Narzędzie `replace` często zawodzi przy plikach >2000 linii (Stack Overflow, brak dopasowania przez białe znaki). 
* **ZAKAZ** podmieniania bloków większych niż 50 linii.
* **NAKAZ** stosowania "chirurgicznych" cięć – podmieniaj unikalne, krótkie fragmenty.
* **ALTERNATYWA:** Jeśli `replace` zawodzi, używaj PowerShell Regex przez `run_shell_command` do precyzyjnego wycinania/edycji linii.
* **WERYFIKACJA:** Po każdej edycji dużego pliku użyj `grep` lub `read_file` (mały zakres), aby potwierdzić, że kod nie został "rozjechany".

---

# DEBUG

## Prawa Architektury (Software)

### Logowanie i Debug
* **DEBUG_LEVEL:** Stosuj poziomy logowania. Surowe zrzuty danych z sensorów mają trafiać do `ATLAS_LOG` tylko jeśli `debug_level >= VERBOSE`.
* **DASHBOARD:** Głównym wizualnym raportem stanu ma być tabela ASCII (3 kolumny).

### Tabela ASCII (Dashboard)
* **Status Sensorów:** Jeśli czujnik jest offline (flag `_ok == false`), jego nagłówek w tabeli `|-[ Nazwa ; Adres ]----` musi zostać całkowicie zamazany kreskami: `|------------------------------------`.

### Skaner I2C
* **Helper Database:** Biblioteka `https://github.com/silasmariusz/I2C_Addr_LS` musi zostać wykorzystana podczas testowania szyny i2c w celu identyfikacji odpowiednich sensorów ale ma mniejszy priorytet niż wszystkie znane adresy z main.cpp obsługiwanych sensorów, które mają pierwszeństwo w identyfikacji i na ich podstawie należy utworzyć listę adresów.
--- 
* **Priorytet:** Sensory stacji (Stevenson, Optics, BMS) MUSZĄ być na początku listy i mieć dopisek w nazwie (np. `( Stevenson )`), aby skaner w pierwszej kolejności identyfikował kluczowe komponenty.

---
*Reszta instrukcji (napięcia, MUX, BMV080, BSEC, ZMOD) pozostaje bez zmian wg. oryginalnej dokumentacji.*

## 7. System Logowania i Sygnalizacji (Non-Blocking Diagnostics)

### 7.1. Asynchroniczna Dioda LED (ledTask)
W projekcie WeatherDNA dioda RGB sterowana jest wyłącznie przez dedykowane zadanie FreeRTOS (`ledTask`). 
*   **ZAKAZ:** Bezpośredniego wywoływania `statusLed.show()` w pętli głównej z użyciem `delay()`.
*   **MECHANIZM:** Wywołanie `triggerLed(level)` tworzy dynamicznie strukturę `LedPattern` i wrzuca ją do kolejki zadań. Dzięki temu procesor nie "marnuje czasu" na czekanie, aż dioda skończy migać, co jest krytyczne dla zachowania rygoru czasowego biblioteki BSEC 3 (`next_call`).

### 7.2. Protokół ATLAS_LOG
Wszystkie komunikaty systemowe muszą przechodzić przez funkcję `ATLAS_LOG(level, verbose, fmt, ...)`.
*   **Integracja:** Każdy log o poziomie błędu (`L_ERROR`, `L_CORRUPTED`, `L_SENSOR_FAILED`) automatycznie wyzwala odpowiedni wzorzec świetlny, pozwalając na diagnostykę "z daleka" (bez podpinania USB).
*   **Poziomy Logowania:**
    *   `L_INFO` - Zielony błysk (operacja poprawna).
    *   `L_ERROR` - Długie czerwone światło (błąd krytyczny).
    *   `L_CORRUPTED` - Fioletowe serie (błędne dane/pamięć).
    *   `L_BOOT_CRASH` - Białe serie (problem przy starcie).

### 7.3. Interaktywne Menu Serwisowe (Serial Console)
System ATLAS udostępnia konsolę diagnostyczną dostępną pod klawiszem **'m'** (Menu).
*   **'s' (I2C Scan):** Uruchamia skaner magistrali I2C (Topologia z Sekcji 2).
*   **'h' (History):** Zrzuca ostatnie 5 wpisów z pamięci EEPROM (0x50).
*   **'r' (Reset):** Wykonuje bezpieczny programowy restart urządzenia.
