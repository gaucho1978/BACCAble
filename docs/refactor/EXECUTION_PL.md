**Kontrakt wykonania pojedynczego PR-a — wersja 2**

Ten dokument wraz z jedną kartą `prs/PR-NNN.md`, [decyzjami](DECISIONS_PL.md) i [kryteriami wspólnymi](ACCEPTANCE_PL.md) stanowi specyfikację zadania. Wykonawca: `gpt-5.6-sol`, reasoning `medium`. Ten poziom jest obsługiwany według [oficjalnej dokumentacji modelu](https://developers.openai.com/api/docs/models/gpt-5.6-sol). Rozmiar kart i sposób pracy są decyzją dla tego repozytorium; nie są gwarancją bezbłędności modelu.

**Granice upoważnienia**

1. Wykonaj dokładnie jeden wskazany PR. Numer karty nie upoważnia do realizacji całej serii, automatycznego scalania, publikowania wydania ani programowania urządzenia.
2. Zaczynaj od aktualnego `main`, po scaleniu i sprawdzeniu wymaganych poprzedników. Domyślnie pracujemy kolejno, bez stosu niescalonych PR-ów. Nie nadpisuj cudzych zmian ani niezwiązanych plików.
3. Czytaj dowolny potrzebny kod. Edytuj wyłącznie pliki i fragmenty wskazane w karcie oraz jej własny test/raport. Zgoda na plik wspólny oznacza wyłącznie deklaracje, wywołania i pola należące do opisywanej funkcji.
4. Nowe pliki wymienione jako `(nowy)` należy utworzyć. Pozostałe powinny istnieć w bazie albo pochodzić z zależności. Brak pliku po scaleniu zależności jest rozbieżnością do zgłoszenia, a nie powodem do tworzenia innej architektury.

   Jeżeli poprzednik przeniósł wskazany w karcie fragment do nowego pliku, dozwolona jest ta sama ograniczona edycja pod nową ścieżką z raportu poprzednika. Zapisz mapping w swoim raporcie. To nie rozszerza zakresu na inne funkcje ani upoważnia do kolejnego przenoszenia. Wyłącznie mechaniczne zastąpienie dawnego odczytu stanu wywołaniem getteru w już wydzielonym module jest dopuszczone na tej zasadzie.
5. Bez dodatkowego zadania: nie zmieniaj formatu CAN/UART, pinów, bitrate, zegarów, poziomów GPIO, progów sterowania, wartości domyślnych, identyfikatorów ustawień ani wersji bibliotek. Wyjątki muszą być wymienione w treści karty.
6. Nie dodawaj RTOS, C++, dynamicznego systemu zdarzeń, pakietów z sieci ani nowego frameworka testowego. C, statyczne bufory i obecna pętla główna pozostają podstawą.
7. Nie dodawaj funkcji produktu wymienionych jako niedokończone w audycie. Nie poprawiaj sąsiedniego kodu, pisowni i formatowania poza zakresem zmiany. Nie łącz ekstrakcji modułu z nowym zachowaniem.
8. Nie modyfikuj biblioteki HAL/CMSIS/FatFs/USB ST w celu ułatwienia testu. Atrapy znajdują się wyłącznie w `tests/host`; produkcja używa rzeczywistej biblioteki. Nie wyłączaj kontroli CI ani sanitizerów, żeby uzyskać zielony wynik.

**Procedura od rozpoczęcia do odbioru**

1. Przeczytaj kartę i te trzy dokumenty. Zapisz SHA bazowego `main`, aktywną konfigurację i stan katalogu roboczego. Sprawdź istnienie wyników wymaganych PR-ów w kodzie i rejestrze; sam numer lub nazwa commita nie wystarczą.
2. Sprawdź warunki wejścia i bramki sprzętowe. Karta może być wykonywana od razu albo wymagać wskazanego dowodu przed zmianą kodu. Brak sprzętu nie blokuje niezależnych kart ani przygotowania dopuszczonego testu hosta.
3. Wyszukaj wskazane symbole w aktualnej bazie. Numery linii w audycie są historyczne. Dopasuj funkcję po nazwie i odpowiedzialności, nie po numerze linii.
4. Zapisz w raporcie krótki plan operacji ograniczony do karty. Dla poprawki przygotuj test konkretnego błędu i potwierdź niepowodzenie na bazie. Dla ekstrakcji przygotuj testy dotychczasowego zachowania przed przeniesieniem kodu.
5. Zmień produkcję i testy. Zachowaj dotychczasowe publiczne wywołania jako cienkie adaptery, jeżeli karta nie zleca ich usunięcia. Utrzymuj działający program po zakończeniu każdej karty.
6. Uruchom test wskazany w karcie, następnie kompletny zestaw hosta i wymagany profil budowania. Zapisz komendy, wersje narzędzi, kody wyjścia i rozmiary. Sprawdź diff względem aktualnej bazy.
7. Własny przegląd: sprawdź wszystkie kryteria, zakres plików, skutki dla pozostałych wariantów, własność buforów i zgodność danych trwałych. Wyniku „nie wykonano” nie zamieniaj na „zaliczone”.
8. Przygotuj raport i opis PR-a. Domyślny punkt zakończenia pracy modelu to zmiana gotowa do review; utworzenie zdalnego PR-a następuje w ramach osobnego polecenia lub istniejącego upoważnienia użytkownika.
9. Po scaleniach testujemy SHA `main`, z którego rzeczywiście powstał firmware. Następna zależna karta rusza dopiero po zaliczeniu odbioru poprzednika. Rebase lub rozwiązanie konfliktu wymaga ponowienia kontroli dotkniętego kodu i pełnego builda.

**Jak testować kod bez kopiowania implementacji**

Host ma kompilować rzeczywiste produkcyjne źródła lub mały wydzielony produkcyjny moduł. Nie kopiuj ciała funkcji do testu. Dozwolone są atrapy wywołań HAL i przechwytywanie wyjść. Jeżeli funkcja statyczna wymaga testu, preferuj sprawdzenie jej publicznego zachowania; mała ekstrakcja jest dozwolona tylko w miejscu podanym w karcie. Nie dodawaj testowych ścieżek zachowania do firmware. Test porównujący starą i nową implementację może użyć starego commita jako oddzielnej bazy, ale nie pozostawia drugiej implementacji produkcyjnej w repozytorium.

**Ograniczenie wielkości**

Budżet każdej karty oznacza liczbę ręcznie napisanych zmienionych linii produkcyjnych (`+` i `-`, bez testów i dokumentacji). Domyślnie: poprawka do 250, adapter do 400, ekstrakcja do 700. Przeniesione bez zmian linie raportuj osobno, używając diffu wykrywającego przenoszenie. Budżet nie jest zachętą do skracania kodu ani pomijania obsługi błędów. Jeżeli trzeba go przekroczyć, przygotuj konkretną propozycję podziału i zakończ status `BLOCKED_SCOPE`; nie rozszerzaj samodzielnie karty. Testy i raport mogą być dłuższe, jeżeli sprawdzają wymagany kontrakt.

**Rozbieżności i nowe znaleziska**

- Błąd poza zakresem: zapisz reproduktor i lokalizację w raporcie jako `FOLLOW_UP`; wykonuj dalej bieżącą kartę, jeśli błąd jej nie blokuje.
- Kryterium sprzeczne z kodem, brak danych ECU, brak miejsca w pamięci, konieczność zmiany innych funkcji: zapisz dowód, najmniejszy potrzebny zakres i status `BLOCKED_SPEC`, `BLOCKED_EVIDENCE` lub `BLOCKED_SCOPE`. Nie zgaduj znaczenia ramek i nie „naprawiaj” testu oczekiwanym wynikiem z własnego kodu.
- Błąd narzędzi lub uprawnień: pokaż rzeczywistą komendę i przyczynę. Obsłuż uprawnienia standardową ścieżką środowiska; nie obchodź ograniczeń innym narzędziem.
- Nie zmieniaj swojej karty, wspólnych kryteriów ani decyzji architektonicznych po to, aby zalegalizować gotową implementację. Zmiana planu jest osobnym zadaniem, z opisem wpływu na zależności.

**Statusy i wymagany raport**

`NOT_STARTED` → `IN_PROGRESS` → `CODE_READY` → `ACCEPTED` → `MERGED` → `VERIFIED_MAIN`. Stan `HW_PENDING` oznacza gotowy kod z brakującym wymaganym odbiorem sprzętowym; nie jest równoważny `ACCEPTED`. Stany blokady są opisane wyżej. W rejestrze zmieniaj tylko wiersz bieżącej karty i dodawaj odnośnik do jej raportu.

Raport `docs/refactor/evidence/PR-NNN.md` musi zawierać: bazowy i wynikowy SHA, zakres plików, powód zmiany, listę dozwolonych zmian zachowania, komendy i wyniki, rozmiary przed/po, osobny wynik sprzętowy z identyfikacją urządzenia, zgodność każdego kryterium, ograniczenia, instrukcję cofnięcia i znaleziska poza zakresem. Nie wpisuj z góry `PASS` w szablonie.

Opis PR-a: problem i nowe zachowanie; dokładny zakres; walidacja; zmiana pamięci; zgodność/rollback; link do karty i raportu. Używaj zwykłego języka i rzeczywistych wyników. Nie oznaczaj całego refaktoru jako ukończonego po ukończeniu jednej karty.
