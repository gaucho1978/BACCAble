**Plan wykonawczy refaktoryzacji BACCAble — wersja 2**

Plan jest przeznaczony do realizacji przez **GPT-5.6 Sol z reasoning medium**, po jednej karcie na sesję i jednym rezultacie na PR. Obejmuje **80 osobnych kart**, które można stopniowo scalać do `main`. Nie rozpoczęto ich realizacji; ten zestaw jest specyfikacją pracy.

Podstawą jest przegląd commita `4e43af1`. Pierwotna analiza i wyniki kompilacji zostały zachowane w [audycie](docs/refactor/AUDIT_PL.md). Kod firmware nie został zmieniony przy przygotowywaniu wersji 2.

**Jak przekazać zadanie modelowi**

Wybierz kartę z gotowymi zależnościami i przekaż [gotowy prompt](docs/refactor/PROMPT_PL.md), zastępując `PR-NNN` jej numerem. Model czyta jedną kartę oraz wspólne reguły; nie potrzebuje całej rozmowy. Wskazanie numeru nie zleca automatycznie kolejnych kart, scalenia, publikacji ani flashowania.

| Dokument | Co ustala |
| --- | --- |
| [Kontrakt wykonania](docs/refactor/EXECUTION_PL.md) | Procedura pracy, zamknięty zakres, dozwolone edycje, blokady, statusy i raport |
| [Decyzje wykonawcze](docs/refactor/DECISIONS_PL.md) | Konkretne interfejsy i zachowania graniczne, UART legacy, format A/B, migracja, wyłączenia |
| [Kryteria akceptacji](docs/refactor/ACCEPTANCE_PL.md) | Komendy, ASan/UBSan, buildy, budżet pamięci, profile sprzętowe, merge i rollback |
| [Karty PR](docs/refactor/prs/PR-001.md) | Cel, zależności, pliki, operacje, wyłączenia, ponumerowane warunki PASS/FAIL |
| [Rejestr wykonania](docs/refactor/STATUS_PL.md) | Aktualny status każdego PR, SHA, dowody i test obrazu z main |
| [Bramki i dowody](docs/refactor/evidence/README.md) | Dane wymagane od rzeczywistego sprzętu; początkowo PENDING |

**Co zmieniło się względem pierwszego planu**

Zamiast dużych etapów typu „uporządkować diagnostykę” są osobne naprawy, testowalne moduły i ich późniejsze integracje. Ekstrakcja funkcji zachowuje zachowanie bieżącego `main`; konkretna poprawka ma osobną kartę FIX oraz reproduktor. Każda karta podaje także to, czego model nie może zmienić.

Zachowujemy C, HAL, pętlę główną i obecny format UART. Nowe źródła początkowo trafiają do istniejących `Core/Src` i `Core/Inc`; nie łączymy przenoszenia katalogów ze zmianą logiki. Protokół UART v2, nowe funkcje, nowe pojazdy i ogólna zmiana reakcji sterowania na utratę ECU pozostają poza tą serią. Usterki odkryte podczas ekstrakcji są raportowane i wymagają osobnego dopisania do planu.

Migracja flash ma konkretną mapę dla potwierdzonego MCU 128 KiB, zapis A/B i zachowane legacy. Wymaga backupu i przygotowanego woluminu FAT: obecne `f_mkfs` nie formatuje mniej niż 128 sektorów, dlatego zmniejszenie partycji bez osobnego przygotowania obrazu byłoby niewystarczające. Model nie ma zgadywać rozmiaru układu ani kasować danych w imię naprawy.

**Kolejność i punkty odbioru**

1. PR-001…003 tworzą powtarzalny build i testy hosta. Następne lokalne poprawki mają już trwały reproduktor.
2. PR-004…030 usuwają wskazane błędy pamięci, walidacji, transportu, zapisu legacy i UDS. PR-y z niezależnymi zależnościami można wybierać według dostępności stanowiska.
3. PR-031…039 obejmują model danych i magazyn. PR-033 warto przygotować od razu po PR-025/026, aby wcześniej ustalić dostępność sprzętu. Brak GATE-FLASH nie blokuje niezależnego porządkowania UI i funkcji.
4. PR-040…065 wydzielają menu, cykl życia i poszczególne funkcje. PR-076 wykonujemy po PR-064, a PR-077 po PR-063: są oddzielnymi małymi zadaniami diagnostyki dyno i koordynacji launch.
5. PR-066…072 poprawiają kolejkowanie, start/wybudzanie, DNA i konfiguracje. PR-078/079 kończą adaptery CAN i ROUTE. PR-073 wykonujemy dopiero po ich zależnościach, mimo niższego numeru.
6. PR-074/075 porządkują opis produktu i mapę testów. PR-080 jest końcowym odbiorem wszystkich poprzedników.

**Zależności karty mają pierwszeństwo przed numeracją.** Nie rozpoczynamy zależnego zadania na niescalonej gałęzi. Stan `VERIFIED_MAIN` wymaga kontroli wyniku scalenia, a nie tylko zielonego builda wcześniejszego commita. Zablokowana karta pozostaje jawnie zablokowana; model nie rozszerza samodzielnie innej karty, aby ją zastąpić.

Przed merge aktywnej zmiany wymagane są jej kontrole programowe i wskazany odbiór sprzętowy. Moduł niepodłączony do runtime może zostać scalony po testach hosta, a włączenie go następuje w odrębnym PR. Nie ma automatycznego zaliczania testów sprzętowych ani publikacji obrazów po każdym merge.

**Katalog kart**

Typy: FIX — naprawa wskazanego zachowania; EXTRACT/REFACTOR — zachowanie zachowane; NEW_MODULE — kod jeszcze niepodłączony; INTEGRATE — podłączenie istniejącego modułu; TOOL/TEST/DOC — infrastruktura lub dokumentacja. Każda karta ma własny budżet i listę dozwolonych plików. Numery w kolumnie zależności wskazują wyniki, które muszą już być obecne na `main`.

| PR | Jeden oczekiwany rezultat | Typ | Wymagane PR-y | Odbiór sprzętowy |
| --- | --- | --- | --- | --- |
| [PR-001](docs/refactor/prs/PR-001.md) | Macierz budowania i mierzalna baza | TOOL | — | Kontrole programowe / dokumentacja |
| [PR-002](docs/refactor/prs/PR-002.md) | Zależności nagłówków i flag w Makefile | FIX | PR-001 | Kontrole programowe / dokumentacja |
| [PR-003](docs/refactor/prs/PR-003.md) | Testy hosta pod sanitizerami w CI | TEST | PR-002 | Kontrole programowe / dokumentacja |
| [PR-004](docs/refactor/prs/PR-004.md) | Nawigacja po widocznych parametrach | FIX | PR-003 | HW-UI |
| [PR-005](docs/refactor/prs/PR-005.md) | Dopełnianie pola tekstowego po NUL | FIX | PR-003 | HW-UI |
| [PR-006](docs/refactor/prs/PR-006.md) | Bezpieczny formatter liczb | FIX | PR-003 | HW-UI |
| [PR-007](docs/refactor/prs/PR-007.md) | Walidacja tokenów szablonu ekranu | FIX | PR-006 | HW-UI |
| [PR-008](docs/refactor/prs/PR-008.md) | Walidacja CAN TX przed kolejką | FIX | PR-003 | HW-USB |
| [PR-009](docs/refactor/prs/PR-009.md) | DLC w dekoderach standardowych CAN | FIX | PR-008 | HW-FEATURE |
| [PR-010](docs/refactor/prs/PR-010.md) | Ścisły parser SLCAN i odrzucanie zbyt długiej komendy | FIX | PR-008 | HW-USB |
| [PR-011](docs/refactor/prs/PR-011.md) | Tryb silent zachowany przy otwarciu CAN | FIX | PR-003 | HW-USB |
| [PR-012](docs/refactor/prs/PR-012.md) | Krótka sekcja krytyczna odbioru CDC | FIX | PR-003 | HW-USB |
| [PR-013](docs/refactor/prs/PR-013.md) | Nieblokujący USB CDC TX | FIX | PR-012, PR-010 | HW-USB |
| [PR-014](docs/refactor/prs/PR-014.md) | Własność bufora UART1 podczas TX | FIX | PR-003 | HW-UART |
| [PR-015](docs/refactor/prs/PR-015.md) | Własność bufora UART2 podczas TX | FIX | PR-014 | HW-UART |
| [PR-016](docs/refactor/prs/PR-016.md) | Atomowe operacje kolejek i wynik enqueue | FIX | PR-015, PR-008 | HW-UART |
| [PR-017](docs/refactor/prs/PR-017.md) | Obsługa poleceń UART poza przerwaniem | EXTRACT | PR-016 | HW-UART |
| [PR-018](docs/refactor/prs/PR-018.md) | Twarde wyłączenie immobilizera | FIX | PR-003 | HW-FEATURE |
| [PR-019](docs/refactor/prs/PR-019.md) | Wyłączenia dyno, hamulców i ESC w C2 | FIX | PR-017 | HW-FEATURE |
| [PR-020](docs/refactor/prs/PR-020.md) | Zakończenie impulsu QV po wyłączeniu | FIX | PR-003 | HW-FEATURE |
| [PR-021](docs/refactor/prs/PR-021.md) | Deterministyczny zapis tablic legacy | FIX | PR-003 | HW-FLASH |
| [PR-022](docs/refactor/prs/PR-022.md) | Walidacja ustawień i wynik SAVE&EXIT | FIX | PR-021, PR-018 | HW-UI |
| [PR-023](docs/refactor/prs/PR-023.md) | Spójna semantyka slotu lusterka BH | FIX | PR-003 | HW-FEATURE |
| [PR-024](docs/refactor/prs/PR-024.md) | Typy wyników disk_ioctl | FIX | PR-003 | HW-FLASH |
| [PR-025](docs/refactor/prs/PR-025.md) | Granice i propagacja błędów diskio | FIX | PR-024, PR-017 | HW-FLASH |
| [PR-026](docs/refactor/prs/PR-026.md) | Prawdziwy kontrakt tylko do odczytu USB MSC | FIX | PR-024 | HW-FLASH |
| [PR-027](docs/refactor/prs/PR-027.md) | Arytmetyka offsetu UDS ze znakiem | FIX | PR-003 | HW-DIAG |
| [PR-028](docs/refactor/prs/PR-028.md) | Sprawdzany katalog parametrów i stabilne ID stron | FIX | PR-007, PR-027 | HW-UI |
| [PR-029](docs/refactor/prs/PR-029.md) | Testowalny stan pojedynczego odczytu UDS | NEW_MODULE | PR-028 | Kontrole programowe / dokumentacja |
| [PR-030](docs/refactor/prs/PR-030.md) | Powiązanie UDS z rzeczywistym żądaniem CAN | INTEGRATE | PR-029, PR-016, PR-004 | HW-DIAG |
| [PR-031](docs/refactor/prs/PR-031.md) | Model sygnałów niezależny od nagłówków HAL | EXTRACT | PR-009, PR-027 | HW-FEATURE |
| [PR-032](docs/refactor/prs/PR-032.md) | Ważność danych na ekranie | INTEGRATE | PR-031, PR-030 | HW-UI + HW-DIAG |
| [PR-033](docs/refactor/prs/PR-033.md) | Potwierdzenie flash i obraz FAT do migracji | EVIDENCE_TOOL | PR-025, PR-026 | Kontrole programowe / dokumentacja |
| [PR-034](docs/refactor/prs/PR-034.md) | Rozłączne partycje i kontrola zgodności woluminu | FIX | PR-033 | HW-FLASH |
| [PR-035](docs/refactor/prs/PR-035.md) | Codec snapshotu i wybór ważnej kopii | NEW_MODULE | PR-028, PR-022, PR-023 | Kontrole programowe / dokumentacja |
| [PR-036](docs/refactor/prs/PR-036.md) | Backend A/B z weryfikacją zapisu | NEW_MODULE | PR-035, PR-034 | Kontrole programowe / dokumentacja |
| [PR-037](docs/refactor/prs/PR-037.md) | Pierwsza migracja pełnego snapshotu C1 | INTEGRATE | PR-036, PR-021, PR-022, PR-028 | HW-FLASH + HW-UI |
| [PR-038](docs/refactor/prs/PR-038.md) | Statystyki i widoczność C1 w jednym magazynie | INTEGRATE | PR-037 | HW-FLASH + HW-UI |
| [PR-039](docs/refactor/prs/PR-039.md) | Migracja ustawień lusterka BH | INTEGRATE | PR-036, PR-023 | HW-FLASH + HW-FEATURE |
| [PR-040](docs/refactor/prs/PR-040.md) | Stabilne identyfikatory i walidacja deskryptorów ustawień | REFACTOR | PR-022, PR-028 | HW-UI |
| [PR-041](docs/refactor/prs/PR-041.md) | Renderowanie menu bez zmiany stanu funkcji | EXTRACT | PR-040, PR-007 | HW-UI |
| [PR-042](docs/refactor/prs/PR-042.md) | Niepodzielny obraz strony wysyłanej przez BH | FIX | PR-017 | HW-UI + HW-UART |
| [PR-043](docs/refactor/prs/PR-043.md) | Widoczność parametrów według profilu i stable ID | INTEGRATE | PR-038, PR-028, PR-004 | HW-UI + HW-FLASH |
| [PR-044](docs/refactor/prs/PR-044.md) | Wydzielenie interpretacji gestów | EXTRACT | PR-009, PR-041 | HW-UI + HW-FEATURE |
| [PR-045](docs/refactor/prs/PR-045.md) | Jedno zdarzenie długiego przytrzymania do zwolnienia | FIX | PR-044 | HW-UI + HW-FEATURE |
| [PR-046](docs/refactor/prs/PR-046.md) | Testowalny cykl życia uśpienia bez zmiany polityki | EXTRACT | PR-017 | HW-WAKE |
| [PR-047](docs/refactor/prs/PR-047.md) | Wydzielenie chronometru | EXTRACT | PR-031, PR-021 | HW-FEATURE |
| [PR-048](docs/refactor/prs/PR-048.md) | Wydzielenie Start & Stop | EXTRACT | PR-009, PR-017 | HW-FEATURE |
| [PR-049](docs/refactor/prs/PR-049.md) | Wydzielenie immobilizera i panic | EXTRACT | PR-018, PR-045 | HW-FEATURE |
| [PR-050](docs/refactor/prs/PR-050.md) | Wydzielenie sterowania szybami | EXTRACT | PR-017, PR-045 | HW-FEATURE |
| [PR-051](docs/refactor/prs/PR-051.md) | Wydzielenie lusterka parkingowego BH | EXTRACT | PR-023, PR-017 | HW-FEATURE |
| [PR-052](docs/refactor/prs/PR-052.md) | Wydzielenie wydechu QV i impulsów pilota | EXTRACT | PR-020, PR-045 | HW-FEATURE |
| [PR-053](docs/refactor/prs/PR-053.md) | Wydzielenie wyboru mapy Schizzaforte | EXTRACT | PR-014, PR-017, PR-022 | HW-UART + HW-FEATURE |
| [PR-054](docs/refactor/prs/PR-054.md) | Rozdzielenie obrazu LED i transmisji DMA | EXTRACT | PR-003 | HW-LED |
| [PR-055](docs/refactor/prs/PR-055.md) | Wydzielenie shift indicator i alertu DPF | EXTRACT | PR-031, PR-045 | HW-LED + HW-FEATURE |
| [PR-056](docs/refactor/prs/PR-056.md) | Wydzielenie ograniczenia migania przebiegu | EXTRACT | PR-009, PR-017 | HW-FEATURE |
| [PR-057](docs/refactor/prs/PR-057.md) | Wydzielenie ustawienia alarmu pasów | EXTRACT | PR-017, PR-009 | HW-DIAG + HW-FEATURE |
| [PR-058](docs/refactor/prs/PR-058.md) | Wydzielenie istniejącego kasowania DTC | EXTRACT | PR-017, PR-019 | HW-DIAG + HW-FEATURE |
| [PR-059](docs/refactor/prs/PR-059.md) | Wydzielenie wirtualnych przycisków ACC | EXTRACT | PR-009, PR-045 | HW-FEATURE |
| [PR-060](docs/refactor/prs/PR-060.md) | Wydzielenie automatycznego wznowienia ACC | EXTRACT | PR-059, PR-031 | HW-FEATURE |
| [PR-061](docs/refactor/prs/PR-061.md) | Wydzielenie wirtualnego przycisku HAS | EXTRACT | PR-017, PR-045 | HW-FEATURE |
| [PR-062](docs/refactor/prs/PR-062.md) | Wydzielenie ESC/TC w C2 | EXTRACT | PR-019, PR-045 | HW-FEATURE + HW-UART |
| [PR-063](docs/refactor/prs/PR-063.md) | Wydzielenie sterownika hamulców przednich C2 | EXTRACT | PR-019, PR-017 | HW-DIAG + HW-FEATURE |
| [PR-064](docs/refactor/prs/PR-064.md) | Wydzielenie maszyny stanów dyno | EXTRACT | PR-019, PR-017 | HW-DIAG + HW-FEATURE |
| [PR-065](docs/refactor/prs/PR-065.md) | Wydzielenie odłączania napędu 4WD | EXTRACT | PR-017, PR-045 | HW-DIAG + HW-FEATURE |
| [PR-066](docs/refactor/prs/PR-066.md) | Odświeżenia ekranu nie wypierają poleceń UART | FIX | PR-016, PR-042 | HW-UART + HW-UI |
| [PR-067](docs/refactor/prs/PR-067.md) | Wznawialne przekazanie ustawień po starcie slave | FIX | PR-066, PR-046 | HW-UART + HW-WAKE |
| [PR-068](docs/refactor/prs/PR-068.md) | Czyszczenie starych operacji na granicy sleep/wake | FIX | PR-067, PR-046, PR-015, PR-013 | HW-WAKE + HW-UART |
| [PR-069](docs/refactor/prs/PR-069.md) | Modyfikacja ramki DNA z ważnego wzorca | FIX | PR-009, PR-062 | HW-FEATURE |
| [PR-070](docs/refactor/prs/PR-070.md) | Budowanie DEBUG BH/C2 i jednoznaczny wybór roli | FIX | PR-001, PR-013 | HW-USB |
| [PR-071](docs/refactor/prs/PR-071.md) | Dostępność funkcji zgodna z pinami i wariantem | FIX | PR-040, PR-070, PR-018, PR-019 | HW-USB + HW-LED |
| [PR-072](docs/refactor/prs/PR-072.md) | Jedna ścieżka kontroli build/CI/release | TOOL | PR-002, PR-003, PR-070, PR-071 | Kontrole programowe / dokumentacja |
| [PR-073](docs/refactor/prs/PR-073.md) | Usunięcie martwych deklaracji po ekstrakcjach | CLEANUP | PR-047, PR-048, PR-049, PR-050, PR-051, PR-052, PR-053, PR-054, PR-055, PR-056, PR-057, PR-058, PR-059, PR-060, PR-061, PR-062, PR-063, PR-064, PR-065, PR-077, PR-079 | Kontrole programowe / dokumentacja |
| [PR-074](docs/refactor/prs/PR-074.md) | Dokumentacja rzeczywistych funkcji i wariantu szeregowego | DOC | PR-053, PR-070 | Kontrole programowe / dokumentacja |
| [PR-075](docs/refactor/prs/PR-075.md) | Rejestr pokrycia funkcji i procedury odbioru | TEST_DOC | PR-074 | Kontrole programowe / dokumentacja |
| [PR-076](docs/refactor/prs/PR-076.md) | Dyno reaguje tylko na odpowiedź bieżącej fazy | FIX | PR-064 | HW-DIAG + HW-FEATURE |
| [PR-077](docs/refactor/prs/PR-077.md) | Wydzielenie koordynacji launch assist w C1 | EXTRACT | PR-063, PR-031, PR-045 | HW-FEATURE + HW-UART |
| [PR-078](docs/refactor/prs/PR-078.md) | Jawne konstruowanie ramek TX i wynik oddania do HAL | FIX_REFACTOR | PR-008, PR-016, PR-030, PR-050, PR-059, PR-060, PR-061, PR-062 | HW-USB + HW-DIAG |
| [PR-079](docs/refactor/prs/PR-079.md) | Wydzielenie istniejącego jednorazowego ROUTE | EXTRACT | PR-009, PR-078 | HW-FEATURE |
| [PR-080](docs/refactor/prs/PR-080.md) | Końcowy odbiór serii i instrukcja rozbudowy | ACCEPTANCE_DOC | PR-001, PR-002, PR-003, PR-004, PR-005, PR-006, PR-007, PR-008, PR-009, PR-010, PR-011, PR-012, PR-013, PR-014, PR-015, PR-016, PR-017, PR-018, PR-019, PR-020, PR-021, PR-022, PR-023, PR-024, PR-025, PR-026, PR-027, PR-028, PR-029, PR-030, PR-031, PR-032, PR-033, PR-034, PR-035, PR-036, PR-037, PR-038, PR-039, PR-040, PR-041, PR-042, PR-043, PR-044, PR-045, PR-046, PR-047, PR-048, PR-049, PR-050, PR-051, PR-052, PR-053, PR-054, PR-055, PR-056, PR-057, PR-058, PR-059, PR-060, PR-061, PR-062, PR-063, PR-064, PR-065, PR-066, PR-067, PR-068, PR-069, PR-070, PR-071, PR-072, PR-073, PR-074, PR-075, PR-076, PR-077, PR-078, PR-079 | Kontrole programowe / dokumentacja |

**Praktyczna definicja ukończenia**

Każda karta kończy się zgodnością ponumerowanych kryteriów z dowodami, raportem rozmiaru pamięci i instrukcją cofnięcia. Cała seria jest zakończona dopiero po PR-080: pokryte są funkcje z audytu, obsługiwane konfiguracje przechodzą CI, dane trwałe mają sprawdzoną migrację, a wymagane scenariusze sprzętowe są przypisane do rzeczywistych urządzeń i SHA.

Model nie dostaje zadania „usuń wszystkie możliwe błędy”. Dostaje ograniczoną listę zmian o sprawdzalnym wyniku. To pozwala ocenić każdą poprawkę przed następną i zatrzymać serię na działającym, przetestowanym `main`.
