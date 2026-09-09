# Analiza i zalozenia nowej architektury

Punkt odniesienia: `6179ada`, 2026-09-09. Zrodla: README, instrukcja EN,
komentarze i tablice protokolow w kodzie oraz trzy workflow GitHub Actions.
Poprzednie materialy `docs/refactor` i `REFACTOR_PLAN_PL.md` nie sa podstawa tej pracy.

## Zakres zachowania

C1: menu, parametry benzyna/diesel, statystyki, immobilizer, start/stop, LED,
shift, ACC/HAS, szyby, wydech, pedal booster, regeneracja, routing diagnostyczny.
C2: ESC/TC, dyno, sterowanie hamulcami, komunikacja z C1.
BH: wyswietlacz, chime, lusterka, maska Race, odometer, dysk USB.
CANable: SLCAN z USB CDC, wszystkie opisane predkosci i ramki RTR.
Funkcje opisane w instrukcji jako niedokonczone (np. odczyt DTC) nie uzyskuja
wymyslonego protokolu. Dane z reverse engineering pozostaja przy dekoderach.

## Ustalenia

| Obszar | Problem potwierdzony w zrodle | Kierunek naprawy |
| --- | --- | --- |
| CAN | Otwarcie nadpisuje silent; DLC bez limitu; utrata przy HAL_BUSY; rzutowanie naglowka RX na TX | Jawna walidacja, kopiowanie pol, kolejki z kontrola wyniku |
| USB | Odczyt niepelnego SLCAN, bledny RTR, blokujacy TX, dostep przed konfiguracja USB, przepelnienie linii | Parser niezalezny od HAL, nieblokujace kolejki, kontrola stanu USB |
| UART | Logika pojazdu i FatFs w ISR, zwalnianie aktywnego bufora TX, bezwarunkowe wlaczanie IRQ | ISR tylko odbiera, glowna petla przetwarza, osobny aktywny bufor |
| UDS | Akceptacja dowolnej odpowiedzi tego samego ECU; unsigned offset dla ujemnych wartosci | Walidacja PCI, SID, DID, dlugosci i podpisanej arytmetyki |
| Katalog | Id odpowiedzi `0xDA18F110` poza zakresem CAN; przecinek zamiast kropki w skali `0.001` | Poprawione dane i test rzeczywistego katalogu dla obu silnikow |
| Menu | Underflow uint8 przy przewijaniu wstecz, niezabezpieczony formatter | Ograniczone iteracje i dlugosci |
| Flash | Zapis niezainicjalizowanych slow statystyk i widocznosci; brak integralnosci | Wersjonowane rekordy, CRC, dwie strony i commit na koncu |
| Dysk | 64 KiB USB obejmuje ustawienia BH; malloc i brak granic; bledne typy ioctl | Oddzielny region, statyczny bufor strony, sprawdzanie zakresu |
| Race | Stale dane syntetycznej ramki 0x384, nadmiar transmisji i niespojny CRC | Reakcja na aktualne ramki, priorytet przetwarzania RX; wymagany test IPC |
| Build | Brak zaleznosci naglowkow i izolacji wariantow; clean zachowuje biblioteki | Oddzielne katalogi, pliki .d, wspolne polecenia lokalnie i w CI |

## Plan wykonania

1. Oddzielic aplikacje, funkcje pojazdu, protokoly, transport i platforme STM32.
2. Zastapic transport i parsery prostymi implementacjami z ograniczonymi buforami.
3. Wydzielic funkcje z monolitow, nadac nazwy domenowe i jawnie przekazywac ramki.
4. Uporzadkowac zapis ustawien, statystyk, widocznosci i lusterek oraz geometrie dysku.
5. Dodac testy hosta z sanitizerami i sprawdzac cztery warianty cppcheck/ARM GCC
   wedlug workflow. Zapisac wynik, rozmiary i ograniczenia weryfikacji.

## Granice weryfikacji

Repo nie zawiera nagran CAN, symulatora ECU ani dostepu do plytek. Kompilacja
i testy hosta nie potwierdzaja timingow fizycznego CAN, zgodnosci IPC ani zachowania
po zaniku zasilania na rzeczywistym STM32. Migotanie Race opisane w instrukcji
wymaga testu z rzeczywistym IPC: nadawanie obok fabrycznego ECU nie gwarantuje
zastapienia jego ramki. Nie wolno deklarowac usuniecia tego objawu tylko na
podstawie poprawnego builda.
