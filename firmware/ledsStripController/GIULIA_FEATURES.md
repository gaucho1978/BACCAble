# Extra dashboard screens, screen rate, warnings and watchdogs

Written for a Giulia 952 2.0 petrol (engine ECU Marelli IAW10JA). Based on V.3.9.4.

**State: builds and host tests only. Nothing here has run on a board or in a car yet.** DID scales marked "unverified" are working assumptions.

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

After an update, pages 47..60 and 65, 66 are visible and 61..64 are hidden until enabled in Params Setup Menu.

## Main menu

- `Hold Mode OFF/MAX/MIN` replaces `Max Hold`.
- `DNA x: ...` binds the current parameter page to the current drive style.
- `Auto Scroll`: off, 3, 5 or 10 s.
- `ECM DTC`: stored and new engine fault count, press to acknowledge (petrol only).
- `Cold Alarm`: coolant limit 75, 70, 80 C or off (petrol only).

These choices are written to flash when the engine is not running.

## Setup menu (all new functions are off by default)

| Entry | Values |
|---|---|
| `Screen Rate` | 500 (as before), 400, 350 ms. One parameter is requested per period |
| `Warnings` | off, text, text and chime. Gates every warning |
| `ECM Watch` | off, all, temperatures, faults |

- Warnings: cold oil with high rpm, low oil pressure, and from the watchdogs coolant hot or cold, hot intercooler air, new engine fault.
- All thresholds are estimates. The oil pressure scale of frame 0x4B2 has not been compared with a scan tool, and the oil warnings also apply to diesel and QV.
- `ECM Watch` sends `22 1003` / `22 1935` at most once per second and `19 01` every 10 s to the engine ECU with tester address F1, also with the menu hidden. Turn it off while using a diagnostic tool. ECU support for `19 01` is unverified.
- The DID 1C16 drop watch is compiled out (`GW_CAC_VERIFIED=0`).

## Fixes

- UDS values below zero are decoded as signed, they used to wrap.
- A reply is shown only if it is a positive answer to the requested DID or PID.
- `floatToStr` no longer writes past its buffer or strips zeros of integers.
- Page visibility buffer is zeroed before it is written to flash.
- `getPreviousVisibleParam` could run past the array when page 0 was hidden.
- `replyId` of parameter 38 (over rpm).

## Saved settings

Slot 33 stays DNA Memory. New: slot 34 warnings, 35 and 36 pages bound to drive styles, 37 auto scroll, 38 ECM watch, 39 cold alarm, 40 screen rate. Blank flash and 0 mean the default. The last word of the page visibility area holds a mark (`0x6A31`).

## Tests

```bash
cd firmware/ledsStripController
hosttest/run.sh Core
```

The tests build the decoder, the screen formatter, the watchdog and the warning logic on the host. They do not cover CAN traffic, the instrument cluster or the chime.
