**Polecenie dla pojedynczej sesji wykonawczej**

Ustaw model `gpt-5.6-sol`, reasoning `medium`, a następnie wklej poniższe polecenie po zastąpieniu `PR-NNN` właściwym numerem. Ustawienie modelu wykonuje użytkownik/środowisko; sam tekst promptu nie przełącza modelu.

```text
Wykonaj wyłącznie PR-NNN z planu refaktoryzacji BACCAble.

Przeczytaj:
- REFACTOR_PLAN_PL.md,
- docs/refactor/EXECUTION_PL.md,
- docs/refactor/DECISIONS_PL.md,
- docs/refactor/ACCEPTANCE_PL.md,
- docs/refactor/prs/PR-NNN.md,
- wymagane raporty poprzedników i bramek wskazane w tej karcie.

Pracuj na aktualnym main po zweryfikowaniu zależności. Przygotuj jedną
lokalną zmianę gotową do review. Nie wykonuj następnego PR-a, nie scalaj,
nie publikuj wydania i nie programuj urządzenia w ramach tego zadania.

Karta i wspólne decyzje określają pełny dopuszczony zakres. Zachowaj
zachowanie, którego karta nie zleca zmienić. Nie naprawiaj sąsiednich
problemów ani nie zmieniaj planu. Jeżeli zakres lub dowody nie wystarczą,
przygotuj konkretny raport blokady zamiast zgadywać.

Uruchom wszystkie wymagane testy możliwe w tym środowisku. Raportuj
oddzielnie testy hosta, build, CI oraz testy sprzętowe. Brak pomiaru nie
oznacza PASS. Zapisz raport w docs/refactor/evidence/PR-NNN.md i zaktualizuj
wyłącznie odpowiadający wiersz docs/refactor/STATUS_PL.md.

Zakończ wynikiem: co zmieniono, które kryteria spełniono, jakie kontrole
wykonano, co pozostaje wymagane do akceptacji oraz jak cofnąć zmianę.
```

Po review, odbiorze i scaleniu użytkownik może zlecić sprawdzenie SHA `main`, a następnie następną kartę. Nie trzeba przekazywać całej historii rozmowy ani wszystkich pozostałych kart; kontekst wymagany do konkretnej pracy jest wymieniony w jej specyfikacji.
