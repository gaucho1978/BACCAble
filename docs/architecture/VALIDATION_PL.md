# Initial architecture validation

Historical report: 2026-09-09, branch `refactor/firmware-architecture`, embedded
version `refactor-validation`. This predates the menu and memory work. Current
results are in [cleanup validation](CLEANUP.md).

## Local results at that stage

| Check | Result |
| --- | --- |
| Host tests | PASS: three executables, ASan and UBSan |
| cppcheck | PASS: C1/C2/BH/CAN |
| ARM GCC | PASS: C1/C2/BH/CAN, ELF/BIN/HEX/MAP |
| Extended C1 | PASS: gasoline + LARGE_DISPLAY + LED + IPC_MY23 |
| BH DEBUG_MODE | PASS |
| actionlint | PASS: all three workflows |
| clang-format check | PASS: application modules and tests |

Tools: Arm GNU Toolchain 15.2.rel1, Apple Clang 21.0.0, cppcheck 2.20.0 and
actionlint 1.7.7. Final compiler, lint and test logs had no warnings or errors;
the DEBUG_MODE pragma message was informational.

## Historical build sizes

Bytes from `arm-none-eabi-size`: program Flash is `text + data`, RAM is
`data + bss`. The `dec` total includes BSS and is not the program Flash usage.

| Variant | text | data | bss | Program Flash | RAM |
| --- | ---: | ---: | ---: | ---: | ---: |
| C1 | 58868 | 1768 | 8336 | 60636 | 10104 |
| C2 | 21516 | 632 | 10096 | 22148 | 10728 |
| BH | 22452 | 572 | 10120 | 23024 | 10692 |
| CAN | 20984 | 456 | 6648 | 21440 | 7104 |
| C1 gasoline/large/LED/IPC23 | 59532 | 1892 | 8744 | 61424 | 10636 |
| BH debug | 16396 | 664 | 6872 | 17060 | 7536 |

All images fit the 65536-byte program region and 16384-byte RAM. Runtime stack
high-water measurement remains a hardware check; the static total is not that measurement.

## Test coverage at that stage

- SLCAN: standard/extended IDs, data/RTR, DLC 0–8, malformed IDs/hex, excess or
  truncated input and insufficient output capacity.
- CAN with HAL substitutes: off-bus behavior, silent mode, prescaler, DLC/ID
  validation, busy retry, full queue/mailboxes, queue reset, forwarding and FIFO.
- USB with ST substitutes: configuration state, TX ownership, full queue,
  fragmented/oversized lines, packet loss, CR resynchronization and interrupt state.
- UDS: PCI/SID/DID, payload bounds, offsets, multi-frame rejection and signed scaling.
- Flash: simulated interruption at every write step, previous-copy recovery,
  corruption/type/size validation and avoiding erase for unchanged data.
- Storage bounds, legacy navigation/formatting edge cases and the actual
  gasoline/diesel parameter catalog.

The tests exercised production drivers with controlled hardware substitutes;
they did not simulate STM32, bus arbitration or an ECU. Menu integration coverage
was expanded in later stages. Legacy formatter/navigation tests were replaced by
current-menu coverage when those unused implementations were removed.

## Hardware work remaining from the original report

No remote Actions, releases, tags, flashing or vehicle testing were performed for
this historical validation. Local workflow commands and workflow syntax were checked.

1. Verify physical Flash capacity and restore settings/mirror calibration after
   the original storage migration.
2. Check CAN ordering/load, UART synchronization, sleep/wake, USB hot-plug and
   interrupted writes on a controlled bench.
3. Exercise supported readings and vehicle features with the actual board set.
4. Capture Race mask behavior with the actual IPC and ECU; corrected checksums
   and removal of synthetic traffic do not establish elimination of flicker.

Unfinished DTC reading, unsupported multi-frame values and demonstration logging
remain functional limits rather than newly implemented capabilities.
