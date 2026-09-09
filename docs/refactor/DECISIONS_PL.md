**Ustalone decyzje wykonawcze — wersja 2**

Te decyzje ograniczają wybory modelu. Ich zmiana wymaga osobnej aktualizacji planu. Oznaczenie `GATE` wskazuje konkretny brak dowodu, który należy uzupełnić; nie pozostawia modelowi swobody zgadywania.

**D01. Układ źródeł i interfejsy**

Nowe moduły powstają na razie płasko w `firmware/ledsStripController/Core/Src` i `Core/Inc`, z prefiksami `feature_`, `vehicle_`, `uds_`, `storage_`, `ui_`, `transport_`. Pozwala to zachować obecne odkrywanie źródeł i ograniczyć mechaniczne zmiany. Nie przenosimy wszystkich katalogów według schematu z pierwszego audytu.

Moduł funkcji posiada prywatny, statyczny stan. Interfejs składa się tylko z potrzebnych operacji `init`, `on_*`, `tick(now_ms)`, `get_status`; nie trzeba tworzyć wszystkich czterech. Testowalne wejścia/wyjścia nie zawierają typów HAL. Adapter przy istniejącym handlerze kopiuje odpowiednie dane i wykonuje zwrócone polecenie. Bez wspólnego „wszystkomającego” kontekstu i bez kopiowania całego stanu samochodu przy każdym ticku.

Prywatny stan dotyczy przebiegu funkcji, nie ustawień zapisanych przez `SetupParam`. Te pozostają u właściciela konfiguracji i są przekazywane jako wejście. W C1 i C2 identycznie nazwane dotychczasowe flagi mogą mieć inne znaczenie: po ekstrakcji rozdzielamy role, nie tworzymy pozornego wspólnego potwierdzenia ECU.

**D02. Zgodność funkcjonalna**

Ekstrakcja zachowuje bajty wysłanych ramek, kolejność operacji, stałe czasowe, warunki progowe (`>` kontra `>=`), domyślne ustawienia i teksty. Znane błędy są zmieniane wyłącznie w kartach `FIX`. Przed ekstrakcją wykonawca zapisuje w jej teście reprezentatywne wejścia i oczekiwane wyjścia bieżącej bazy oraz wykaz wykorzystywanych stałych. Jeżeli zachowanie nie jest jednoznaczne, blokuje zmianę tego zachowania, ale może przygotować opis rozbieżności.

**D03. Transport i czas**

Pętla główna jest właścicielem logiki aplikacji. ISR odbiera dane do ograniczonego bufora albo zgłasza zakończenie/błąd operacji. Sekcja krytyczna zachowuje i odtwarza `PRIMASK`; nie obejmuje parsera, wysyłki, formatowania, FatFs ani oczekiwania.

Aktywny bufor TX jest niezmienny od przyjęcia przez HAL do callbacku końca/abortu. Pełna kolejka odrzuca nową pozycję i zwraca wynik; nigdy nie nadpisuje aktywnej. Karty priorytetyzacji mogą scalać tylko niewysłane odświeżenia ekranu. Używamy `uint32_t` i odejmowania `now - started` dla czasu, przy interwałach krótszych niż połowa zakresu licznika. Nie zmieniamy częstotliwości ramek bez osobnej karty.

**D04. Protokół C1/C2/BH**

W tej serii pozostają obecne identyfikatory, padding, długość UART 19/25 i okna półdupleksu. Poprawiamy pamięć, kolejkowanie, ponawianie po odrzuceniu enqueue i synchronizację dostępnych ustawień po resecie. Nie ponawiamy automatycznie `TOGGLE` po niepewnym wyniku transmisji. Dla statusów rozróżniamy `UNKNOWN`, `REQUESTED` i stan raportowany przez procesor; raport procesora nie jest dowodem potwierdzenia przez ECU.

CRC, numerowanie żądań i nowy protokół UART są wyłączone z tej serii. Wymagałyby odrębnego planu zgodności i aktualizacji trzech procesorów. Obecny plan nie obiecuje wykrywania każdej korupcji pakietu ani wykonania polecenia dokładnie jeden raz. Przyjęcie tej granicy pozwala scalać małe zmiany do `main` i testować również mieszane wersje firmware.

**D05. Formatowanie i puste dane**

`floatToStr`: pojemność 0 oznacza brak zapisu; 1 oznacza sam NUL. Dla większej pojemności wynik ma najwyżej `capacity-1` znaków, jest dopełniony spacjami i zakończony NUL. Poprawne, mieszczące się wartości zachowują dotychczasowy sposób zaokrąglania i obcinania zbędnych zer. NaN pozostawia puste pole; infinity oraz wartość niemieszcząca się lub niemożliwa do bezpiecznej konwersji daje pole z `#`. Maksymalna obsługiwana precyzja to 3, zgodnie z używanymi szablonami; większa daje `#`, bez pętli o nieograniczonym koszcie.

Brak widocznych parametrów jest stanem UI, a nie poprawnym indeksem tablicy. Wyszukiwanie zwraca `bool` i zapisuje indeks przez argument wyjściowy tylko po sukcesie. Bez widocznych stron ekran pokazuje `NO PARAMS`, dopełnione do aktualnej szerokości; nawigacja wstecz pozwala wrócić do menu. Odczyt UDS nie jest wtedy uruchamiany. Nie wymuszamy włączenia losowego parametru.

**D06. UDS odczytowy**

Jeden aktywny odczyt `ReadDataByIdentifier` na magistralę C1. Kontekst przechowuje ECU request/reply ID, DID, ID parametru, generację widoku i czas. Zapytanie staje się oczekujące dopiero po przyjęciu do CAN TX; czas wysłania jest osobnym zdarzeniem, jeżeli adapter je udostępnia. Timeout odpowiedzi jest liczony od oddania ramki HAL do transmisji, nie od zmiany strony. Do czasu dodania takiego zdarzenia adapter musi używać ograniczonego, opisanego timeoutu kolejki i odpowiedzi; nie zakłada, że enqueue oznacza wysłanie.

Akceptujemy tylko pasującą odpowiedź single-frame: poprawny PCI i zadeklarowana długość mieszcząca się w DLC, usługa `0x62`, ten sam DID i ECU. Zmiana strony unieważnia aktualizację starego widoku; wynik może zostać zapisany pod swoim ID parametru. Negatywna odpowiedź `0x7F 0x22` kończy bieżącze żądanie jako błąd; także NRC `0x78` w tej wersji nie przedłuża go bez końca. Następny odczyt może nastąpić w normalnym cyklu. Brak ISO-TP i automatycznego włączania niezaimplementowanych wpisów.

Domyślne ograniczenia klienta: timeout oczekiwania w kolejce 500 ms, odpowiedzi 500 ms; po timeoutcie nie wysyłaj ponownie tej samej pary ECU/DID przez 500 ms. To decyzja nowej obsługi błędu i wymaga profilu HW-DIAG przed akceptacją integracji. Sam UDS nie zawiera numeru naszej transakcji: bardzo późnej odpowiedzi na identyczny DID po ponowieniu nie da się bezwarunkowo odróżnić. Nie deklarujemy takiej gwarancji w testach.

**D07. Dostępność i ustawienia**

`PERMANENTLY_DISABLE_IMMO` jest twardym zakazem uruchamiania immobilizera. `DISABLE_IMMOBILIZER` zachowuje udokumentowane znaczenie ustawienia początkowego, z możliwością późniejszego włączenia. Nie utożsamiaj tych makr. Trzy flagi `DISABLE_*` funkcji C2 wykluczają wykonanie odpowiadających im poleceń.

`LAUNCH_ASSIST_THRESHOLD` jest nazwą kanoniczną istniejącej opcji użytkownika. Zapisana poprawna wartość ma pierwszeństwo przed domyślną. `PEDAL_MAP_POWER` jest walidowane po interpretacji jako `int8_t`, zakres −10…10. Nie dodajemy nowych domyślnie aktywnych funkcji. USB i LED na PA11 nie mogą zostać jednocześnie aktywowane także przez ustawienie odczytane z flash.

**D08. Flash: dokładny wariant docelowy i bramka wejściowa**

Aktywacja nowego magazynu wymaga `GATE-FLASH`: potwierdzonego oznaczenia MCU i 128 KiB flash, strony 2048 B, kopii legacy ustawień oraz danych dysku z używanego urządzenia. Bez tego wolno realizować poprawki lokalne i testy hosta; nie wolno zmieniać aktywnych adresów ani kasować stron. Urządzenie z 64 KiB wymaga osobnego planu mapy i pozostaje poza aktywacją tej migracji.

Docelowa mapa dla potwierdzonego 128 KiB; przedziały mają prawy koniec wyłączny:

| Zakres | Przeznaczenie |
| --- | --- |
| `0x08000000..0x08010000` | Program, nadal 64 KiB |
| `0x08010000..0x0801D800` | FAT: 54 KiB, 108 sektorów po 512 B |
| `0x0801D800..0x0801E000` | Magazyn A, jedna strona |
| `0x0801E000..0x0801E800` | Magazyn B, jedna strona |
| `0x0801E800..0x08020000` | Trzy zachowane strony legacy; nie kasować podczas migracji |

Każda rola przechowuje jeden kompletny snapshot swoich danych w A albo B. Nie tworzymy osobnych par stron na każdą kategorię ustawień. Magazyn C1 obejmuje konfigurację, dwa rekordy czasu i widoczność obu profili. BH obejmuje pozycje/flagę lusterka. C2 nie dostaje nowych ustawień trwałych.

Istniejący wolumin 128-sektorowy nie staje się poprawnym woluminem 108-sektorowym przez zmianę stałej. Przed aktywacją na używanym urządzeniu wymagany jest backup i przygotowany offline obraz FAT 108 sektorów, zapisany wyłącznie do jego partycji przez operatora. Narzędzie przygotowania obrazu powstaje w PR-033; bez automatycznego flashowania. Firmware nie formatuje uszkodzonego/niezgodnego woluminu przy starcie. Zgłasza `NOT_READY`, pozostawia resztę funkcji działającą i nie zmienia zawartości. Nowe A/B wolno zapisać na urządzeniu dopiero po zakończeniu przygotowania dysku, ponieważ te strony mogły wcześniej zawierać plik.

Obecne `ff.c:f_mkfs` odrzuca woluminy mniejsze niż 128 sektorów. PR-033 nie może więc użyć go do formatowania 108 sektorów ani usuwać tego ograniczenia z biblioteki. Narzędzie offline tworzy pusty obraz FAT12: sektor 512 B, jeden sektor/klaster, jeden sektor zarezerwowany, jedna FAT o długości jednego sektora, root 32 wpisy (dwa sektory), 108 sektorów razem, media `0xF8`, pierwsze bajty FAT `F8 FF FF`, sygnatura boot `55 AA`, brak tablicy partycji. Pozostałe pola BPB/EBPB muszą być spójne; obraz jest weryfikowany przez rzeczywiste `f_mount`, zapis i odczyt pliku z repozytoryjnym FatFs na hoście. Nie przerabiamy istniejącego, zapełnionego woluminu przez przycięcie pliku obrazu.

**D09. Format snapshotu i atomowość**

Format little-endian, bez zapisywania struktury C przez `memcpy`: magic 4 B `BACS`, wersja `uint16=1`, rola `uint16` (C1=1, BH=3), generacja `uint32`, długość payload `uint16`, reserved `uint16=0`, CRC32 4 B; payload od offsetu 20. Commit `uint16=0xA55A` leży w ostatnich dwóch bajtach strony. Nieużyte bajty pozostają `0xFF`.

CRC32/ISO-HDLC obejmuje bajty 0…15 oraz payload (pomija pole CRC, padding i commit), polynomial reflected `0xEDB88320`, init/xorout `0xFFFFFFFF`; wektor `123456789` daje `0xCBF43926`. Nagłówek i długość są sprawdzane przed użyciem danych. Payload jest uporządkowaną listą TLV: klucz `uint16`, długość `uint16`, bajty wartości. Powtórzony klucz, nadmiarowa długość, zła rola/wersja odrzucają rekord. Nieznany klucz jest pomijany przy odczycie wersji 1 i zachowany przy kolejnym zapisie tylko jeśli mieści się w ustalonym buforze; inaczej zapis zwraca błąd, zamiast cicho go usunąć.

Klucze: C1 `0x0100 + legacy flash_index` dla ustawień (wartość uint16), `0x0201/0x0202` dla rekordów w ms (uint16, zachowanie sentinelów legacy), `0x0301/0x0302` dla list stabilnych ID widocznych stron benzyna/diesel (liczba uint16 + ID uint16). BH `0x0401..0x0409` odpowiada dziewięciu dotychczasowym slotom, z poprawioną semantyką slotu 5. Katalog ID stron utrwala PR-028; kolejność UI nie zmienia ID.

Klucz `0x03FF` (uint16=1) oznacza zakończenie przełączenia API statystyk/widoczności C1 w PR-038. Przed jego pojawieniem się źródłem tych dwóch kategorii jest nadal legacy; konfiguracja już korzysta z A/B. Marker i aktualne dane muszą trafić do jednego atomowego snapshotu.

Zapis: odczytaj ważny snapshot → scal zmianę w RAM → wybierz nieaktywną stronę → erase → zaprogramuj nagłówek/payload/CRC półsłowami → odczytaj i sprawdź → zaprogramuj commit jako ostatni → zweryfikuj. Poprzedniej ważnej strony nie kasuj w tej transakcji. Wybór nowszej generacji wykorzystuje różnicę modulo 32 bity, przy odległości <2^31; równe generacje wybierają A. Po awarii ma pozostać stary albo nowy pełny snapshot, nigdy ich mieszanka.

Migracja jest wznawialna i zawsze czyta nietknięte legacy, jeśli nie ma poprawnego nowego snapshotu. Pierwszy snapshot C1 musi od razu zawierać wszystkie trzy kategorie; PR-y podłączające kolejne API nie mogą ich zgubić. Downgrade firmware nie synchronizuje nowych zmian do legacy: odzyska stan sprzed migracji. Ten skutek trzeba opisać operatorowi; nie obiecujemy bezstratnego downgrade.

**D10. Freshness i funkcje sterujące**

PR o ważności danych najpierw dodaje metadane i dotyczy prezentacji. Progi odczytów UDS wynikają z D06. Dla natywnych sygnałów okres i dozwolona przerwa wymagają `GATE-SIGNALS`: śladu magistrali z czasem i tabeli sygnał → timeout zaakceptowanej w raporcie PR-032. Nie przyjmuj dowolnego „1 s dla wszystkiego”. Brak danych pozwala dodać znacznik „nie otrzymano”, ale blokuje aktywację wygaszania według nieustalonego progu.

Zmiana zachowania sterującego po utracie sygnału/ECU wymaga osobnej karty FIX z konkretnymi ramkami zwolnienia i warunkami. Karty ekstrakcji zachowują dotychczasowe zachowanie i zapisują wykryte luki jako osobne znaleziska. Nie wysyłaj wymyślonej „bezpiecznej ramki” do hamulców, ACC ani ECU.

**D11. Co świadomie pozostaje poza serią**

Pełny nowy protokół UART, nowe usługi UDS/ISO-TP, nowa funkcjonalność logowania, zdalny rozruch, odczyt DTC, dokończenie `eujot`, zmiana elektroniki, nowe modele samochodu, zmiana progów launch/ACC i automatyczna publikacja. Wydzielenie istniejącego kasowania DTC pozostaje w zakresie. Nie usuwamy widocznych opcji tylko na podstawie tego, że wyglądają na eksperymentalne; ich stan opisuje dokumentacja.
