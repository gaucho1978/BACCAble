# Weryfikacja refactoru

Data: 2026-09-09. Branch: `refactor/firmware-architecture`.
Wersja osadzona w sprawdzanych obrazach: `refactor-validation`.

## Wynik lokalny

| Kontrola | Wynik |
| --- | --- |
| `make -C tests test` | PASS, trzy programy testowe, ASan i UBSan |
| `make FLAVOR=C1 lint` | PASS |
| `make FLAVOR=C2 lint` | PASS |
| `make FLAVOR=BH lint` | PASS |
| `make FLAVOR=CAN lint` | PASS |
| ARM GCC: C1, C2, BH, CAN | PASS, ELF/BIN/HEX/MAP |
| C1: gasoline + LARGE_DISPLAY + LED + IPC_MY23 | PASS |
| BH: DEBUG_MODE | PASS |
| `actionlint` | PASS dla wszystkich trzech workflow |
| `clang-format --dry-run --Werror` | PASS dla modulow aplikacji i testow |

Narzedzia: Arm GNU Toolchain 15.2.rel1, Apple Clang 21.0.0, cppcheck 2.20.0,
actionlint 1.7.7. Koncowe logi kompilatora, cppcheck i testow nie zawieraly
ostrzezen ani bledow. Komunikat pragma o DEBUG_MODE jest informacyjny.

## Rozmiary

Wartosci w bajtach, z `arm-none-eabi-size`. Flash programu to `text + data`;
statyczny RAM to `data + bss`. Stos i sterta potrzebuja dodatkowego zapasu.
Nie nalezy porownywac sumy `dec` z limitem Flash, poniewaz obejmuje tez BSS.

| Wariant | text | data | bss | Flash programu | Statyczny RAM |
| --- | ---: | ---: | ---: | ---: | ---: |
| C1 | 58868 | 1768 | 8336 | 60636 | 10104 |
| C2 | 21516 | 632 | 10096 | 22148 | 10728 |
| BH | 22452 | 572 | 10120 | 23024 | 10692 |
| CAN | 20984 | 456 | 6648 | 21440 | 7104 |
| C1 gasoline/large/LED/IPC23 | 59532 | 1892 | 8744 | 61424 | 10636 |
| BH debug | 16396 | 664 | 6872 | 17060 | 7536 |

Kazdy obraz miesci sie w obszarze programu 65536 bajtow oraz RAM 16384 bajtow.
Pomiar maksymalnego stosu w rzeczywistym przebiegu pozostaje testem sprzetowym.

## Zakres testow

- SLCAN: standard/extended, data/RTR, wszystkie DLC 0..8, bledne identyfikatory,
  znaki hex, nadmiarowe dane, skrocone wejscia i za maly bufor wyjsciowy.
- CAN ze stubem HAL: off-bus, silent, prescaler, walidacja DLC/ID,
  zachowanie ramki po HAL_BUSY, pelna kolejka, brak wolnego mailboxa,
  reset kolejki przy zamknieciu, jawne RX->TX, FIFO i bledy startu peryferium.
- USB ze stubem ST: brak konfiguracji, kopiowanie danych TX, zachowanie
  aktywnego bufora, zapelnienie kolejki, fragmentacja linii, nadmierna dlugosc,
  utrata pakietu i synchronizacja do CR, zachowanie poprzedniego PRIMASK.
- UDS: PCI/SID/DID, dlugosc danych, bledny offset, odrzucenie multi-frame,
  ujemny offset surowej wartosci i skalowanie.
- Flash: zasymulowane przerwanie kazdego kroku zapisu, zachowanie poprzedniego
  rekordu, wykrywanie korupcji, typ i rozmiar rekordu, brak kasowania bez zmian.
- Granice regionow Flash i dysku, przewijanie strony wstecz oraz brak
  widocznych stron, male bufory formattera, NaN, nieskonczonosci i duze liczby.
- Rzeczywisty katalog gasoline/diesel: liczby stron, terminacja nazw, zakres
  indeksow, schema zadan, identyfikatory odpowiedzi i regresja skalowania.

Testy transportu sprawdzaja kod sterownika z kontrolowanym HAL, nie symuluja
STM32 ani arbitrazu magistrali. Test UDS dekoduje ramke; nie jest emulatorem ECU.
Nie ma automatycznego odtworzenia wszystkich interakcji menu ani funkcji pojazdu.

## Co pozostaje do sprawdzenia na sprzecie

Nie uruchamiano zdalnych GitHub Actions i nie publikowano release ani tagow.
Sprawdzono lokalne polecenia wykorzystywane przez workflow oraz jego skladnie.
Nie flashowano plytek i nie wykonywano testow w samochodzie.

1. Zweryfikowac identyfikowany rozmiar Flash, ponownie ustawic opcje oraz
   pozycje lusterek; stare rekordy i geometria dysku nie sa kompatybilne.
2. Na stanowisku CAN sprawdzic C1/C2/BH: DLC, kolejnosc ramek, obciazenie,
   synchronizacje UART, sleep/wake, hot-plug USB oraz zapis przy zaniku zasilania.
3. Sprawdzic menu, wszystkie udostepnione parametry obu silnikow, LED,
   immobilizer, start/stop, ESC/TC, dyno, wydech, szyby i lusterka.
   Funkcje ingerujace w pojazd najpierw sprawdzic na kontrolowanym stanowisku.
4. Zarejestrowac Race mask z rzeczywistym IPC i ECU: usuniecie syntetycznej
   ramki i poprawa CRC nie dowodza usuniecia migotania. Firmware nie jest
   przezroczysta bramka zastapujaca fabryczne ramki.

Nie dodano protokolow niepotwierdzonych w researchu. Dawne stuby odczytu DTC,
niezaimplementowane wartosci ISO-TP i demonstracyjny zapis pliku pozostaja
ograniczeniami funkcjonalnymi, a nie deklarowanymi nowymi mozliwosciami.
