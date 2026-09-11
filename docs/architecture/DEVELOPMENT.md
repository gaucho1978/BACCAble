# Development constraints and lessons

These are constraints from the implemented firmware, checked against `b29102a`.
They replace the obsolete refactor plan and intermediate reports. Use the
[architecture](README.md), [menu guide](MENU_UX.md) and
[upstream porting guide](UPSTREAM_PORTING.md) when changing the relevant area.

## Preserve established behavior

| Area | Constraint and reason |
| --- | --- |
| CAN/UART/USB | Retain copied buffers until transmission completes; retry rejected queue submissions. A queued command is not an ECU acknowledgment. Preserve interrupt state and keep vehicle actions/Flash writes outside interrupts. |
| Decoding | Check frame type, ID, length and diagnostic correlation before reading data. Preserve signed offsets, scales and units; reject malformed or stale values rather than inventing measurements. |
| Menu | Preserve page identities **and table order**: favorites use IDs, visibility uses indices. The gasoline catalog already fills its 64 slots; increasing capacity requires preference migration. Keep both display widths working. |
| Storage | Keep settings slots and serialized record identities stable. Changed layouts need explicit migration and old-record tests. Preserve CRC, dual copies, final commit marker and unchanged-write suppression. |
| Memory | Use fixed buffers and naturally aligned structures. The catalog uses 28-byte definitions; CAN queue entries store compact metadata; validity flags use bits. Do not expand these without measuring the whole image. |
| Display and DMA | Keep one settings-screen buffer and coalesce waiting screens without replacing active transfers. The LED PWM buffer cannot simply be shortened without adapting DMA. USB and the C1 strip share hardware pins. |
| Automation | Preserve fresh-input checks, manual overrides and restoration of changes owned by the feature. Keep unsupported signal interpretations out of active control paths. |

## Boundaries to consider when touching related code

- Normal feature processing advances through short periodic calls. ELM diagnostics
  deliberately owns the buses and uses synchronous waits; it is not a concurrent
  background menu feature. Preserve session cleanup and timeouts when changing it.
- `protocol/elm327.c` currently includes command parsing, routing and transaction
  handling. Split responsibilities only when the change benefits from it, with
  equivalent command/response tests.
- Raw IBS caching in `app/main.c` and BH RPM decoding in `features/parking.c` are
  exceptions to the usual `vehicle` decoding boundary. Account for their callers
  and freshness checks if moving them; no separate implementation is needed.

## Evidence for a change

Use production modules in host tests with substitutes only at hardware boundaries.
Test the changed behavior and meaningful failure paths. Run the applicable checks
from the [build guide](../../firmware/baccable/MAKEFILE.md); compare linked Flash
and RAM with the [dated build measurements](UPSTREAM_BUILD_SIZES.md).

Builds and sanitizer tests do not establish actual IPC timing, Race-mask behavior,
parking/actuator response, sleep/wake behavior, USB-host compatibility or maximum
stack use. For an affected hardware check, record the source commit, firmware
flags, board/vehicle identity, procedure, observed result and capture provenance.
Distinguish synthetic fixtures from captured traffic and pending checks from PASS.

Keep source revisions, compatibility changes and test results with each change.
