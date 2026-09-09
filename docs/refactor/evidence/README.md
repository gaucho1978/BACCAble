**Dowody wykonania i bramki wejściowe**

Ten katalog nie zawiera jeszcze wyników realizacji nowego planu. Przegląd bazowy jest w [AUDIT_PL.md](../AUDIT_PL.md). Tymczasowe reproduktory z audytu nie zastępują trwałych testów w przyszłych PR-ach.

| Bramka | Stan początkowy | Wymagany artefakt |
| --- | --- | --- |
| GATE-FLASH | PENDING | `hardware-flash.md`: identyfikacja MCU, rozmiar/page, adresy, backup z sumami, plan przygotowania FAT, raport operatora z PR-033 |
| GATE-SIGNALS | PENDING | `signals.md`: ślad CAN ze znacznikami czasu, pochodzenie, okresy i zaakceptowane timeouty per sygnał, PR-032 |
| GATE-DNA | PENDING | `dna-384.md`: pełne ramki 0x384 dla używanych trybów, reguły licznika/CRC i dozwolona maska, PR-069 |

Ślady syntetyczne oznaczaj `synthetic`, a przechwycone `captured`; oba wymagają opisu źródła i SHA. Nie wymyślaj danych ze sprzętu. Jeżeli ślad zawiera dane identyfikujące pojazd, do repozytorium włączaj tylko pola potrzebne do testu i zapisz sposób anonimizacji bez zmiany testowanej semantyki.

Raport per PR jest tworzony dopiero podczas jego realizacji według [szablonu](../ACCEPTANCE_PL.md). Dopiski dotyczące nowych usterek nie rozszerzają automatycznie zakresu żadnej karty.
