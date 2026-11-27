# Extra dashboard screens, screen rate, warnings and watchdogs

Written for a Giulia 952 2.0 petrol (engine ECU Marelli IAW10JA). Based on V.3.9.4.

**State: builds and host tests only. Nothing here has run on a board or in a car yet.** DID scales marked "unverified" are working assumptions.

The new pages, `ECM DTC` and `Cold Alarm` need the engine type set to gasoline. The manuals do not describe these features yet.

## Boards

- C1: all features.
- BH: one fix, a new dashboard line is taken only between two writes, so a 350 or 400 ms screen rate cannot mix two texts.
- C2: no functional change.

## New parameter pages (petrol)

| Pages | Screens | Notes |
|---|---|---|
| 47..53 | gearbox and water temp, DID 1C16 and intercooler air, MAP and boost, air mass and fuel flow, knock 1..4, gear and torque | 1C16 meaning and DID 1942 fuel flow are unverified |
| 54..60 | spark correction per cylinder (signed), knock (2 bytes), fuel rail pressure, oil and MultiAir oil, EGT and catalyst, idle target and rpm | rail pressure: which DID is measured is unverified |
| 61..64 | fuel trims, ambient and water temp, timing advance and load, lambda | OBD service 01, **functional requests on 0x18DB33F1** |
| 65, 66 | warm-up timer, instant L/h and L/100 km | |

After an update of a board already set to gasoline, pages 47..60 and 65, 66 are visible and 61..64 are hidden until enabled in Params Setup Menu.

Known issue: the default for new pages is applied at power on, and only with gasoline selected. After a change from diesel to gasoline, check pages 47..66 in Params Setup Menu: on a blank board 61..64 stay visible until the next power cycle, and if the page selection was saved with diesel selected, pages 57..66 stay hidden.

## Main menu

Press RES to change an entry.

- `Hold Mode OFF/MAX/MIN` replaces `Max Hold`. It is not saved.
- `DNA x: ...` binds the last shown parameter page to the current drive style, or removes the binding if that page is already bound. The bound page opens when the drive style changes.
- `Auto Scroll`: off, 3, 5 or 10 s. It runs with the parameter page on screen, the engine running and Hold Mode off.
- `ECM DTC`: stored and new engine fault count, press to acknowledge (petrol only). It needs `ECM Watch` set to all or faults.
- `Cold Alarm`: coolant limit 75, 70, 80 C or off (petrol only).

The drive style pages, `Auto Scroll` and `Cold Alarm` are written to flash when the engine is not running.

## Setup menu (all new functions are off by default)

Changes are kept with `SAVE&EXIT`.

| Entry | Values |
|---|---|
| `Screen Rate` | 500 (as before), 400, 350 ms. One parameter is requested per period |
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

Slot 33 stays DNA Memory. New: slot 34 warnings, 35 and 36 pages bound to drive styles, 37 auto scroll, 38 ECM watch, 39 cold alarm, 40 screen rate. Blank flash and 0 mean the default. The last word of the page visibility area holds a mark (`0x6A31`).

## Tests

```bash
cd firmware/ledsStripController
hosttest/run.sh Core
HOSTFLAGS=-DGW_CAC_VERIFIED=1 hosttest/run.sh Core
```

The script needs zsh and clang with AddressSanitizer.

The tests build the decoder, the screen formatter, the watchdog and the warning logic on the host. They do not cover CAN traffic, the instrument cluster or the chime.
