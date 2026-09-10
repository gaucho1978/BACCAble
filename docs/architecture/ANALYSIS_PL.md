# Original architecture analysis

Historical baseline: `6179ada`, 2026-09-09. Sources were the README, English
manual, source comments and protocol tables, and the three GitHub Actions workflows.
The older `docs/refactor` material and `REFACTOR_PLAN_PL.md` were not this plan's basis.
See [architecture](README.md) and [cleanup validation](CLEANUP.md) for the current state.

## Intended behavior to preserve

C1: menu, gasoline/diesel readings, performance records, immobilizer, Start/Stop,
LEDs, shift indicator, ACC/HAS, windows, exhaust, pedal booster, regeneration alerts
and diagnostic routing. C2: ESC/TC, dyno, brake control and C1 communication.
BH: display, chimes, mirrors, Race mask, odometer and USB disk.
CANable: USB CDC SLCAN, documented bitrates and RTR frames.
Unfinished features such as DTC reading were not assigned invented protocols.

## Findings and intended repairs

| Area | Confirmed source issue | Repair direction |
| --- | --- | --- |
| CAN | Opening overwrote silent mode; unchecked DLC; loss on HAL_BUSY; RX-to-TX header cast | Validation, explicit field copies and checked queues |
| USB | Partial SLCAN parsing, incorrect RTR, blocking TX, access before configuration, line overflow | Independent parser, nonblocking queues, USB-state checks |
| UART | Vehicle logic and FatFs in interrupts; active TX buffer released; unconditional IRQ enable | Receive-only ISR, main-loop processing, owned active buffer |
| UDS | Unrelated replies from the same ECU accepted; unsigned negative offsets | Validate PCI/SID/DID/length and signed arithmetic |
| Catalog | Invalid CAN reply ID `0xDA18F110`; comma instead of decimal point in scale | Correct data and test both real catalogs |
| Menu | Backward navigation underflow and unbounded formatting | Bounded iteration and output |
| Flash | Uninitialized statistics/visibility words and no integrity check | Versioned dual-page records, CRC, final commit marker |
| Disk | USB disk overlapped BH settings; malloc, missing bounds and wrong ioctl types | Separate region, static page buffer, range checks |
| Race | Stale synthetic `0x384`, excess transmissions and inconsistent checksum | React to current reports, prioritize RX; validate with physical IPC |
| Build | Missing header dependencies/variant isolation; incomplete cleaning | Separate outputs, dependency files and shared local/CI commands |

## Implementation sequence

1. Separate application, vehicle features, protocols, transport and STM32 support.
2. Replace transport/parser paths with bounded implementations.
3. Split monolithic features, name their responsibilities and pass frames explicitly.
4. Define persistence and disk geometry for settings, records, visibility and mirrors.
5. Add sanitizer host tests and verify four variants with cppcheck and ARM GCC;
   record sizes and verification limits.

## Original verification limits

The repository did not provide vehicle CAN recordings or an ECU simulator.
Builds and host tests cannot confirm physical CAN timing, IPC compatibility or
power-loss behavior on STM32. Race-mask flicker requires real IPC/ECU captures:
transmitting alongside an ECU does not guarantee replacement of its message.
A successful build alone does not establish that the symptom is fixed.
