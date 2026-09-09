# Architektura firmware BACCAble

Analiza i plan: [ANALYSIS_PL.md](ANALYSIS_PL.md).
Polecenia lokalne i CI: [MAKEFILE.md](../../firmware/baccable/MAKEFILE.md).
Wyniki weryfikacji: [VALIDATION_PL.md](VALIDATION_PL.md).

## Podzial odpowiedzialnosci

Kod aplikacji jest w `firmware/baccable`. Nie wymaga generatora ani kontenera DI.
Kazdy wariant jest pojedynczym programem C, z jedna petla zdarzen.

| Katalog | Odpowiedzialnosc |
| --- | --- |
| `app` | Inicjalizacja, kolejnosc pracy w petli, konfiguracja kompilacji, polecenia miedzy plytkami |
| `transport` | Kolejki i sterowniki CAN/UART; bez dekodowania funkcji samochodu |
| `protocol` | SLCAN: parser niezalezny od HAL i adapter transportu; checksum ramek pojazdu |
| `vehicle` | Routing wedlug CAN ID i dekodery ramki, pogrupowane domenowo |
| `diagnostics` | Katalog parametrow, odczyty lokalne, korelacja UDS, walidacja odpowiedzi |
| `features` | Okresowe funkcje pojazdu, wyswietlacz, formatowanie, nawigacja |
| `settings` | Definicje menu, zmiana ustawien, serializacja trwalych danych |
| `state` | Jawnie nazwane struktury stanu: telemetria, podwozie, komfort, lusterka itd. |
| `storage` | Dwu-stronicowe rekordy, geometria Flash, blokowy dysk FatFs |
| `platform` | Zasilanie, GPIO, LED, zegar, przerwania i pozostala integracja STM32 |
| `USB_DEVICE` | Integracja CDC/MSC z biblioteka ST |
| `third_party` | FatFs i printf; oddzielone od kodu aplikacji |
| `Drivers`, `Middlewares` | Dostarczone biblioteki ST/CMSIS |

## Przeplyw wykonania

`main` inicjalizuje platforme i funkcje wariantu. Kazda iteracja odbiera ograniczona
partie CAN, obsluguje odebrane wiadomosci UART, wykonuje funkcje okresowe,
obsluguje USB, nadawanie CAN i sygnalizacje LED. Dekodery dostaja wskaznik do
naglowka i 8-bajtowego bufora ramki, zamiast czytac globalna ostatnia wiadomosc.
Kazdy dekoder sprawdza wymagane DLC przed dostepem do danych.

CAN TX kopiuje dane do kolejki. HAL_BUSY nie usuwa ramki. CAN RX i TX maja
rozne typy naglowkow, dlatego forwarding kopiuje pola jawnie.
UART ISR zbiera ramki; dyspozytor polecen i zapis Flash dzialaja w glownej petli.
Aktywne bufory UART i USB pozostaja wlasnoscia transportu do zakonczenia TX.
Sekcje krytyczne zachowuja poprzedni PRIMASK. Po utracie pakietu USB parser
odrzuca niepelna linie do nastepnego CR, zamiast laczyc niepowiazane fragmenty.

Stan pozostaje statyczny, podzielony na struktury domenowe, bez alokacji sterty.
`application_state.h` jest naglowkiem zbiorczym integracji funkcji; moduly czyste
(SLCAN, UDS, rekordy, formatowanie liczb, wyszukiwanie strony) go nie potrzebuja.
Ich testy wykonuja ten sam kod, ktory trafia do firmware.

## Rozszerzanie

- Nowa ramka: dekoder w odpowiednim pliku `vehicle`, minimalne DLC, deklaracja
  w `frame_handlers.h` i wpis w dyspozytorze. Dane z researchu zostaja przy ID.
- Nowa funkcja okresowa: plik w `features`, funkcja `*_process` oraz wywolanie
  z `powertrain_process`, `chassis_process` lub `body_process`.
- Parametr: `ParameterDefinition` w `diagnostics/parameter_catalog.c`, nastepnie
  `ParameterPage` z dwoma indeksami i szablonem `$x.yf` albo `$enum`.
  Pole `raw_offset` jest podpisane; wynik to `(raw + raw_offset) * scale + scaled_offset`.
- UDS akceptuje pojedyncza ramke `0x62` tylko dla aktualnego ECU, DID i strony.
  Obsluga ISO-TP multi-frame nie jest implementowana; dodanie takich parametrow
  wymaga najpierw transportu ISO-TP i testow na zarejestrowanych odpowiedziach.
- Ustawienie: wpis menu, pole `SettingsState`, wartosc domyslna oraz przypisanie
  w `settings/persistence.c`. Przy zmianie ukladu rekordu zmien jego identyfikator.

## Trwale dane i zgodnosc

Program zajmuje pierwsze 64 KiB Flash. Dysk USB zajmuje nastepne 52 KiB
(`0x08010000..0x0801cfff`), rekordy ostatnie 12 KiB (`0x0801d000..0x0801ffff`).
Kazdy rekord ma dwie strony po 2048 bajtow, CRC32, generacje, typ i wersje.
Znacznik commit jest zapisywany jako ostatni. Niezmieniona wartosc nie kasuje Flash.
Ustawienia/statystyki/widocznosc C1 oraz pozycje lusterek BH maja odrebne typy.

Persistence i MSC wymagaja ukladu z **128 KiB raportowanego Flash**.
Na rzeczywistym 64 KiB MCU funkcje te odmawiaja odczytu/zapisu, a ustawienia
dzialaja w RAM. Nie zakladamy dostepnosci nieudokumentowanej pamieci.
Poprzedni format ustawien i geometria dysku nie sa kompatybilne. Przed
aktualizacja zachowaj potrzebne pliki i spisz ustawienia; po aktualizacji
ustaw je ponownie i skalibruj pozycje lusterek. Aktualizuj caly zestaw plytek.

Dwu-stronicowy zapis chroni rekordy, nie caly system FAT: przerwanie zapisu
sektora pliku nadal moze uszkodzic plik. MSC jest tylko do odczytu z hosta;
logowanie lokalne jest wykonywane w glownej petli. Dotychczasowy demonstracyjny
zapis `hello.txt` pozostaje demonstracyjny, nie jest rejestratorem ruchu CAN.

## Narzedzia

Makefile jest jedynym wspieranym opisem kompilacji. Stare wygenerowane projekty
CubeIDE usunieto, poniewaz wskazywaly nieistniejace pliki i ustawienia builda.
`baccable.ioc` pozostaje referencja konfiguracji sprzetu, nie generatorem nowej
architektury. W IDE nalezy otworzyc projekt oparty o zewnetrzny Makefile.
Format kodu definiuje glowny `.clang-format`; bibliotek third-party nie formatuj.

CI uruchamia testy hosta z ASan/UBSan, cppcheck i ARM GCC dla C1/C2/BH/CAN.
Stable i beta korzystaja z tego samego workflow przed publikacja. DEBUG_MODE
wyklucza wariant CAN, bez produkowania fikcyjnego pliku ELF.
