# Final code cleanup and validation

Local validation: 2026-09-10, Arm GNU Toolchain 15.2.rel1,
`VERSION=menu-ux`, branch `refactor/firmware-architecture`.

## Review changes

- Added concise English purpose comments to application functions, feature
  callbacks, USB integration and interrupt handlers. Removed stale commented-out
  implementations and translated remaining application comments. Vendor libraries
  and copyright notices retain their original content.
- Removed the unused old visibility array, navigation helpers, number formatter,
  dashboard entry points, persistence wrappers, blocking LED helpers, obsolete
  error timestamps and declarations without implementations.
- Removed runtime fields and buffers that were never read, including the former
  screen buffer and unused seatbelt/remote-start message copies. Persisted settings
  retain their existing slots and serialization.
- Removed eight empty CAN handlers and one handler that only wrote an unread
  transmission-temperature field. Their useful observations remain in
  [signal notes](REFERENCE_SIGNALS.md). Current coolant and gearbox screens still
  use diagnostic readings; shift-indicator behavior remains in its active handler.
- Removed unused parameter unit/precision fields: presentation comes from screen
  templates. The catalog now uses 28 bytes per definition, down from 36 immediately
  before cleanup, saving 800 bytes across 100 entries. Historical metadata is in
  [parameter reference](PARAMETER_REFERENCE.md). Parameter identities and decoding
  fields are preserved.
- Removed the tests and build inputs belonging to deleted helpers. The production
  dashboard formatter, menu and transport paths retain sanitizer coverage.
- Converted architecture and extension guides to English and marked historical
  reports explicitly. Existing `_PL` links remain valid.

Reference checks included callbacks registered in tables, HAL overrides, FatFs
entry points, startup vectors and conditional builds. A symbol with no direct
application call is not necessarily unused. SVC/PendSV vector stubs are retained
and explain why their bodies are empty.

## Checks

| Check | Result |
| --- | --- |
| Host tests | PASS: six executables with ASan/UBSan |
| cppcheck | PASS: C1, C2, BH, CAN |
| Baseline ARM builds | PASS: C1, C2, BH, CAN |
| 24-character builds | PASS: C1, C2, BH with MY23; C1 also gasoline and LED |
| BH DEBUG_MODE | PASS |
| Application formatting and whitespace | PASS |
| Optional C1 pedal serial adapter | Link limit exceeded; see below |

The compiler reports serial LTO partition compilation for C1 builds; this is a
build-performance warning, not a source diagnostic. The optional adapter build
exceeds the 64 KiB program region by 584 bytes with
`-DACT_AS_SCHIZZAFORTE_SERIAL_CONTROLLER`. It is not a validated release image.
The same adapter flags also fail at baseline commit `a71b4c7`, exceeding Flash
by 2344 bytes. The remaining overflow is an existing optional-build limitation;
resolving it needs separate feature/size work. Ordinary C1 pedal-controller
features remain included in the successful C1 builds.

## Image sizes after cleanup

Bytes from `arm-none-eabi-size`. Flash is `text + data`; RAM is `data + bss`,
including the linker reservation for heap/stack. This is not a runtime stack
high-water measurement. The program region is 65536 bytes; RAM is 16384 bytes.

| Variant | Program Flash | RAM | Free program Flash |
| --- | ---: | ---: | ---: |
| C1 | 63252 | 9268 | 2284 |
| C2 | 22168 | 10264 | 43368 |
| BH | 23208 | 10236 | 42328 |
| CAN | 21404 | 6632 | 44132 |
| C1-menu-large | 63220 | 9460 | 2316 |
| C2-menu-large | 22180 | 10368 | 43356 |
| BH-menu-large | 23196 | 10356 | 42340 |
| BH-cleanup-debug | 17160 | 7088 | 48376 |

Compared with the immediately preceding [memory stage](MEMORY_PL.md), cleanup
saves 932 bytes of Flash and 168 bytes of RAM on baseline C1. Extended C1 saves
1112 bytes of Flash and 176 bytes of RAM. Alignment and LTO affect whole-image
sizes, so those totals are not a simple sum of removed fields.

Comments do not consume device memory. Dead-data removal increases available
memory; it is not a measured improvement in menu latency or vehicle performance.
No device flashing, vehicle testing or remote GitHub Actions run was performed
for this cleanup. Hardware timing, IPC interaction and peak stack usage remain
outside the evidence provided by these host tests and builds.
