# Extra dashboard screens, warnings and watchdogs

Written for a Giulia 952 2.0 petrol (engine ECU Marelli IAW10JA). Based on V.3.9.4.

**State: partly tested on one car (Giulia 952 2.0 petrol), owner's report.** With the final code on C1 (BH and C2 on an earlier build of the branch) the engine starts with no new dashboard errors, the immobilizer status is as before, the new Setup entries and `SAVE&EXIT` work, and the rail pressure page, the three value pages and `Auto Scroll` work. On an earlier build the parameter pages showed values and the `Cold Alarm` entry worked. Values were not compared with a scan tool in these reports. Not tested: drive style page binding, `Hold Mode`, `Warnings` and the chime, `ECM Watch`, the OBD pages 61..64, diesel. DID scales marked "unverified" are working assumptions.

The new pages, `ECM DTC` and `Cold Alarm` need the engine type set to gasoline. The manuals do not describe these features yet.

## Boards

- C1: all features.
- BH: one fix, a new dashboard line is taken only between two writes, so a line arriving during a write (for example after quick menu presses) cannot mix two texts.
- C2: no functional change.

## New parameter pages (petrol)

| Pages | Screens | Notes |
|---|---|---|
| 47..53 | gearbox and water temp, DID 1C16 and intercooler air, MAP and boost, air mass and fuel flow, knock 1..4, gear and torque | 1C16 meaning and DID 1942 fuel flow are unverified |
| 54..60 | spark correction per cylinder (signed), knock (2 bytes), fuel rail pressure (DID 1946), oil and MultiAir oil, EGT and catalyst, idle target and rpm | rail pressure: whether DID 1946 is the measured or the target value is unverified |
| 61..64 | fuel trims, ambient and water temp, timing advance and load, lambda | OBD service 01, **functional requests on 0x18DB33F1** |
| 65, 66 | warm-up timer, instant L/h and L/100 km | |

After an update, pages 47..60 and 65, 66 are visible and 61..64 are hidden until enabled in Params Setup Menu. The default is applied with either fuel selected, and saving the page selection keeps the bits of all pages.

Both fuel types share one visibility bit per page number, as upstream already does. On a board set to diesel, pages 0..56 keep the diesel choice, so check pages 47..56 after a change to gasoline.

## Three value pages and page order

| Page | Screen | Values |
|---|---|---|
| 67 | `U84 O86 W89 °` | UniAir oil, engine oil, water |
| 68 | `O86 W89 G71 °` | engine oil, water, gearbox |
| 69 | `M0.3 B1.02 T27` | manifold pressure, boost, intercooler air |
| 70 | `P2.2 O86 W89 °` | oil pressure, oil, water |
| 71 | `13.9V -2 A 85 %` | battery voltage, current, charge |
| 72 | `G4 T250 B1.8` | gear, torque, boost |
| 73 | `E585 C490 T27` | exhaust gas, catalyst, intercooler air |

Pages are shown with three value pages first, then two value pages, then single ones. Page numbers do not change, so saved visibility and drive style bindings stay. Params Setup Menu lists the combined pages, `SAVE & EXIT`, the single pages and `SAVE & EXIT` again, then starts over. Either entry saves everything.

Values the board already has are refreshed on every 500 ms screen period. Requested values take turns, one request per period.

## Main menu

Press RES to change an entry.

- `Hold Mode OFF/MAX/MIN` replaces `Max Hold`. It is not saved.
- `DNA x: ...` binds the last shown parameter page to the current drive style, or removes the binding if that page is already bound. The bound page opens when the drive style changes.
- `Auto Scroll`: off, 3, 5 or 10 s. It runs with the parameter page on screen, the engine running and Hold Mode off.
- `ECM DTC`: stored and new engine fault count, press to acknowledge (petrol only). It needs `ECM Watch` set to all or faults.
- `Cold Alarm`: coolant limit 75, 70, 80 C or off (petrol only).

The drive style pages, `Auto Scroll` and `Cold Alarm` are written to flash when the engine is not running. This write changes only those settings: Setup menu changes still need `SAVE&EXIT`.

## Setup menu (all new functions are off by default)

Changes are kept with `SAVE&EXIT`.

| Entry | Values |
|---|---|
| `Warnings` | off, text, text and chime. Gates every warning |
| `ECM Watch` | off, all, temperatures, faults |

### Warnings

| Warning | Raised when | Needs `ECM Watch` |
|---|---|---|
| Oil cold | oil below 60 C and above 3500 rpm for 0.3 s | no |
| Oil pressure low (critical) | below 1.5 bar at 600 rpm or more for 2 s | no |
| Coolant hot (critical) | 108 C or more for 5 s | temperatures |
| Coolant cold | below the `Cold Alarm` limit for 10 s, after 20 min of running and 2 min above 80 km/h | temperatures |
| Intercooler air hot | 65 C or more above 30 km/h for 10 s | temperatures |
| New engine fault | the fault count grows | faults |

Critical warnings are also shown with the dashboard menu hidden. The chime is requested only with `Warnings` set to text and chime.

- All thresholds are estimates. The oil pressure scale of frame 0x4B2 has not been compared with a scan tool, and the oil warnings also apply to diesel and QV.
- `ECM Watch` sends `22 1003` / `22 1935` at most once per second and `19 01` every 10 s to the engine ECU with tester address F1, also with the menu hidden. It pauses during Read Faults, Clear Faults and while ELM327 is active. Turn it off while using an external diagnostic tool. ECU support for `19 01` is unverified.
- The DID 1C16 drop watch is compiled out (`GW_CAC_VERIFIED=0`).

## Fixes

- UDS values below zero are decoded as signed, they used to wrap.
- A reply is shown only if it is a positive answer to the requested DID or PID.
- `floatToStr` no longer writes past its buffer or strips zeros of integers.
- Page visibility buffer is zeroed before it is written to flash.
- `getPreviousVisibleParam` could run past the array when page 0 was hidden.
- `replyId` of parameter 38 (over rpm).
- Scale of parameter 54 (gasoline particulate) was written `0,001`, so it was always multiplied by 0. It is now `0.001`.

## Saved settings

Slot 33 stays DNA Memory. New: slot 34 warnings, 35 and 36 pages bound to drive styles, 37 auto scroll, 38 ECM watch, 39 cold alarm. Blank flash and 0 mean the default. The last word of the page visibility area holds a mark (`0x6A32`).

## Tests

```bash
cd firmware/ledsStripController
hosttest/run.sh Core
HOSTFLAGS=-DGW_CAC_VERIFIED=1 hosttest/run.sh Core
```

The script needs zsh and clang with AddressSanitizer.

The tests build the decoder, the screen formatter, the watchdog and the warning logic on the host. They do not cover CAN traffic, the instrument cluster or the chime.
