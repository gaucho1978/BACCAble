**Wspólne kryteria pracy, testowania i scalania**

Każda karta dziedziczy wszystkie właściwe kryteria poniżej. Testy wyszczególnione w karcie uzupełniają je. Konkretne ścieżki skryptów w tym dokumencie są kontraktem do utworzenia w PR-001/003; nie oznaczają, że narzędzia już istnieją.

**A. Warunki rozpoczęcia**

- Wskazano jeden numer PR; istnieje aktualna karta i jej zakres jest jednoznaczny.
- Zależności mają status `VERIFIED_MAIN` i są obecne na bazowym SHA. Karty bez zależności sprzętowych można wykonywać niezależnie od oczekiwania na inne pomiary.
- Wymagane bramki mają rzeczywisty raport; domyślne `PENDING` nie oznacza zgody ani wyniku pozytywnego.
- Zapisano wynik testu/buildu bazy. Wcześniejsza awaria spoza karty jest opisana, a nie ukryta. Nie dodajemy nowych wyjątków CI w celu przejścia bieżącego zadania.

**B. Profile automatyczne i ich komendy**

Komendy uruchamiane z korzenia repozytorium. `PR-NNN` i nazwa testu są literalnie zastępowane numerem/nazwą z karty. Katalog `.refactor-out/` jest ignorowany przez git od PR-001. Raporty JSON, logi i binaria są artefaktami CI; do dokumentacji trafiają odnośniki, SHA i podsumowanie, nie binaria.

| Profil | Polecenie / wymaganie |
| --- | --- |
| B4 | `python3 scripts/refactor/build.py --profile base --out .refactor-out/PR-NNN --report .refactor-out/PR-NNN/build.json` — C1, C2, BH, CANable; oddzielne katalogi obejmujące HAL i USB |
| BX | Ten sam skrypt z `--profile extended`: B4 oraz C1 DEBUG, C1 LARGE_DISPLAY, C1 Schizzaforte serial. C2/BH DEBUG są raportowane jako znany błąd do PR-070, potem muszą przechodzić. CANable+DEBUG pozostaje jawnie niedozwolone. |
| H | `make -C tests/host test SANITIZE=1` — wszystkie testy istniejące na danym SHA, bez zależności od ARM/HAL i bez sprzętu |
| H:nazwa | `make -C tests/host test SANITIZE=1 TEST=nazwa` — test wskazany w karcie; nieznana nazwa musi kończyć się błędem, nigdy pustym sukcesem |
| L | Istniejący cppcheck dla czterech ról w CI, rozszerzany na nowe produkcyjne pliki. Bez nowych globalnych suppressions. |
| D | `git diff --check` oraz sprawdzenie listy zmienionych plików względem karty. Dla nowego pliku sprawdzić także whitespace przed dodaniem go do indeksu. |
| R | Porównanie referencyjnych wejść/wyjść na bazie i zmianie: identyczne ramki CAN/UART, teksty, GPIO i decyzje czasowe z wyjątkiem wyraźnie opisanych napraw |
| M | Raport `text`, `data`, `bss`, rozmiaru obrazów oraz `.su`, na tym samym toolchainie i zestawie flag przed/po |

Build C1 przed wprowadzeniem skryptu PR-001 można wykonać z `firmware/ledsStripController` przez `CFLAGS='-DRELEASE_FLAVOR=C1_FLAVOR -DC1_FLAVOR=1' make -j4 all BUILD_DIR=/tmp/baccable-c1-audit`. Analogicznie C2/BH/CAN z ich nazwą `*_FLAVOR`. Flagi przekazuje się przez środowisko, żeby zachować dopisywane flagi Makefile. Nie używać `make flash` w walidacji programowej.

PR-001 zapisuje dokładną wersję dostępnego ARM GCC w manifeście, a PR-072 przypina tę samą w CI/release. Wartości audytu z 9 września służą porównaniu orientacyjnemu; podstawą akceptacji jest ponownie zmierzony bazowy SHA na tym samym toolchainie. Wersję firmware przy porównywaniu obrazów należy ustalić na stały tekst, ponieważ wbudowany SHA/dirty zmienia binarium.

**C. Warunki przejścia automatycznej walidacji**

1. Wszystkie wymagane role budują się od zera; `.elf`, `.bin`, `.hex` i `.map` należą do tego samego SHA i konfiguracji. Brak nowych ostrzeżeń w zmienionych modułach.
2. H przechodzi z ASan i UBSan (`halt_on_error=1`), a proces zwraca niezero przy błędzie. Testy nie są zależne od kolejności, zegara ściennego, internetu ani rzeczywistych przerwań.
3. Dla FIX istnieje przynajmniej jeden test nieprzechodzący na bazie z powodu naprawianej usterki; dla MOVE/EXTRACT porównanie R przechodzi przed i po. Wyjątki typu sam dokument lub błąd Makefile mają wskazany inny dowód w karcie.
4. Wyjścia testu są sprawdzane niezależnie od implementacji: konkretne bajty, liczba wywołań, stan, kod błędu. Test „nie wywaliło się” nie wystarcza dla zmiany funkcjonalnej.
5. Zapis do bufora sprawdzamy dla pojemności 0/1/granicznej i z otaczającymi znacznikami; parsowanie dla skrócenia w każdym miejscu istotnego pola. Kolejka: pusta/pełna/zawinięcie/aktywny transfer/błąd/ponowne wejście.
6. Wszystkie zmienione terminy mają test tuż przed, dokładnie na i po granicy oraz przy przejściu `UINT32_MAX → 0`. Nie wymuszamy zmiany `>` na `>=` w ekstrakcji.
7. Program mieści się w 65536 B (`text + data` dla obecnego linkera). Wynik ma co najmniej 512 B wolnego regionu programu dla konfiguracji, która podlega akceptacji; pogorszenie ponad ten limit blokuje PR i wymaga osobnej optymalizacji lub zmiany planu. Nie powiększać regionu linkera, aby ominąć kontrolę.
8. RAM mieści się w zadeklarowanym regionie; `.data + .bss` nie jest pomiarem maksymalnego stosu. Nowy bufor >256 B wymaga uzasadnienia, raportu RAM i analizy `.su`. Aktywacja magazynu/DMA wymaga pomiaru stosu na sprzęcie z co najmniej 256 B niezużytej przestrzeni w badanym najgorszym scenariuszu; nie jest to dowód dla wszystkich możliwych obciążeń.
9. Brak zmiany pinów, bitrate, biblioteki lub danych trwałych poza dopuszczonym wyjątkiem karty. Zmiana deklaracji `extern` ma wskazanego jednego właściciela, bez równoległej aktywnej kopii starego i nowego stanu.

**D. Profile odbioru sprzętowego**

Wykonuje je osoba z urządzeniem/stanowiskiem i zapisuje wynik dla konkretnych SHA C1/C2/BH, wariantu MCU, szerokości ekranu i używanego zestawu ECU. Model może przygotować firmware i procedurę; nie wpisuje wyniku za operatora. Dla kart zmieniających aktywne sterowanie profil sprzętowy jest wymagany przed statusem `ACCEPTED`. Po scaleniu powtarzamy co najmniej smoke test na obrazie z `main`.

| Profil | Minimalna procedura i oczekiwany wynik |
| --- | --- |
| HW-UI | Uruchomienie, wejście do menu, przejście całej listy w obie strony, zmiana ustawienia i powrót. Powtórzyć dla używanych 18/24 znaków i benzyna/diesel. Brak resetu, brak mieszania fragmentów stron; dokładne scenariusze naprawy z karty. |
| HW-USB | 100 powtórzeń pakietu `V\rV\r`, poprawna i skrócona komenda SLCAN, ruch CAN podczas komunikacji, dziesięć odłączeń/podłączeń USB. Brak zawieszenia/resetu; błędna komenda nie nadaje CAN; ramki zaakceptowane przez kolejkę odpowiadają wejściu. Przeciążenie ma jawny licznik/kod odrzucenia. |
| HW-UART | Przez 10 min wymuszać odświeżanie ekranu, statusy i legalne polecenia na trzech procesorach. Zebrać ślad TX/RX; brak nadpisania ramek/kolizji wynikających ze zmiany, brak nadawania slave poza dotychczasowym oknem; przepełnienie zgodne z polityką karty. |
| HW-DIAG | Dwa różne DID tego samego ECU, zmiana strony w trakcie, brak odpowiedzi, odpowiedź spóźniona/zła/negatywna. Tylko pasująca aktualizuje właściwy parametr, brak pętli ponowień. Zapisane opóźnienia potwierdzają przyjęte timeouty; brak takich pomiarów oznacza `HW_PENDING`. |
| HW-FLASH | Backup, potwierdzona mapa i wolumin; zapis/odczyt po restarcie, zanik zasilania podczas erase/program/commit na stanowisku, ponowna migracja. Stary albo nowy pełny snapshot; legacy i sąsiednie partycje niezmienione. Co najmniej 20 cykli na stanowisku, równolegle test hosta każdego kroku zapisu. |
| HW-FEATURE | Scenariusze konkretnej karty przy tych samych wejściach co przed ekstrakcją. Rejestrować wejściowe i wyjściowe ramki oraz czasy; identyczna sekwencja i liczba impulsów, opóźnienie obsługi nie większe niż bazowe + jedna zmierzona maksymalna iteracja pętli. Nie zaliczać porównania na podstawie samych tekstów menu. |
| HW-LED | Pełna inicjalizacja, minimalna/maksymalna wartość, zmiana efektu w trakcie DMA, wyłączenie i ponowne włączenie, restart. Poprawny obraz, brak zapisu do aktywnego bufora DMA i brak zajęcia pinów USB. |
| HW-WAKE | Dziesięć cykli bezczynność → uśpienie → aktywność; reset samego BH i C2; wariant z podłączonym USB. Zachowane progi kodu, brak starych komend po restarcie, ustawienia ponownie dostarczone po zapełnieniu kolejki. Rejestrować GPIO resetu i pobór prądu. |

Nie ma wspólnego obowiązku wykonywania manewrów drogowych. Test hamulców, ACC, immobilizera i napędu wymaga właściwego stanowiska/odtworzenia sygnałów; model nie może samodzielnie zastąpić pomiaru na sprzęcie domysłem. Pełna walidacja samochodowa może być oddzielnym odbiorem operatora.

**E. Scalanie i stopniowe wdrażanie**

Karta opisuje najmniejszy rezultat gotowy do scalenia. Kod nieaktywny, np. przetestowany codec bez podłączenia do runtime, może zostać scalony bez udawania, że migracja jest już wdrożona. Domyślną ścieżkę przełącza osobna karta z wymaganym odbiorem. Nie scalaj aktywnej zmiany z obowiązkowym, niewykonanym testem sprzętowym.

Po merge uruchom B4/H/L na SHA `main`; dla zmiany runtime dołącz wynik smoke testu właściwego profilu. Dopiero wtedy wpisz `VERIFIED_MAIN`. Kolejny PR powinien mieć krótką listę tego, co dziedziczy po poprzedniku. Konflikt, nowe zachowanie lub nowy błąd wymagają sprawdzenia, a nie automatycznego aktualizowania wszystkich oczekiwań testowych.

Cofnięcie zwykłej karty: revert jej commita na nowej gałęzi i powtórzenie profili. Cofnięcie karty formatu flash: procedura D08/D09 oraz backup; nowego stanu nie ma automatycznie w legacy. Cofnięcie integracji może zostawić niewykorzystywany, przetestowany moduł — jego usunięcie nie jest wymagane w awaryjnym rollbacku.

**F. Szablon zapisu wyniku**

```text
PR: PR-NNN
Status: IN_PROGRESS / CODE_READY / HW_PENDING / ACCEPTED / ...
Baza main: <SHA>
Wynik: <SHA lub informacja: jeszcze bez commita>
Zakres produkcji: <pliki i symbole>
Dozwolona zmiana zachowania: <konkret lub brak>
Reproduktor / charakterystyka bazy: <komenda, wynik>
H/B4/BX/L/D/R/M: <osobno PASS, FAIL albo NOT_RUN + powód>
Flash i RAM przed/po: <rola, toolchain, flagi, rozmiary>
Kryteria karty: <każde ID i dowód>
Sprzęt: <profil, operator, wariant, SHA, scenariusze i wyniki>
Rollback: <procedura, wpływ na zapisane dane>
Poza zakresem: <znaleziska lub brak>
Merge main / test po merge: <SHA i dowód, dopiero po wykonaniu>
```
