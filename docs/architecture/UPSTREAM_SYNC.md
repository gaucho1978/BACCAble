# Upstream feature integration

This integration extends the modular v5-beta firmware. It does not replace the
architecture with either upstream's monolithic application.
For subsequent integrations, follow [the upstream porting guide](UPSTREAM_PORTING.md).

## Baseline and source history

The local integration base is `94552903ca3276e95a4797ccb0ed1186a8979c63`;
the port is recorded in local commit `b29102a`. The source heads below describe
that completed comparison, not a claim that upstream has no newer commits.
Source heads were fetched and checked again on **2026-09-11**:

| Source | Reviewed revision | Commit date |
| --- | --- | --- |
| [gaucho1978 master](https://github.com/gaucho1978/BACCAble/commit/02b2fd8b7f16d0e399077df4dd7026090564ecbd) | `02b2fd8b7f16d0e399077df4dd7026090564ecbd` | 2026-09-06 |
| [netzmark master](https://github.com/netzmark/BACCAble/commit/a3ca08246d2818d39a00848587f8ec5f61fa986c) | `a3ca08246d2818d39a00848587f8ec5f61fa986c` | 2026-09-04 |
| [netzmark stable-master-3.0.14+](https://github.com/netzmark/BACCAble/commit/5854eda3261cc004d507b100a5627f222d83eef1) | `5854eda3261cc004d507b100a5627f222d83eef1` | 2026-08-31 |

The common ancestor between the local base and each source master is
`d74281080e99b1129c7ec119bab7179539959ff9`, **2026-03-28, V.3.2.4**.
The first following gaucho feature commit is `352c828`, **2026-05-31**,
introducing maximum hold. Netzmark has independent functional changes from
February 2026, so the shared March ancestor alone does not describe its missing
features. Its stable tag was compared separately as well.

## Functional mapping

Paths below are relative to `firmware/baccable`.

| Upstream behavior | Integration location | Result |
| --- | --- | --- |
| Maximum hold | `diagnostics/parameter_cache.c`, `features/menu.c` | Holds numerical maxima; resets on page/engine changes or toggling; stale readings remain unavailable. Status values stay live. |
| Additional measurements | `diagnostics/parameter_catalog.c` | Cylinder 5/6 ignition, oil level in mm, total and cylinder 1–4 misfires, ECM battery SOC and raw IBS bytes. |
| Corrected SOC and fuel rate | `diagnostics/parameter_catalog.c` | Battery SOC pages use ECM DID `19BD`; diesel fuel rate uses the newer gaucho scale `0.0004`. Native IBS SOC remains available for comparison. |
| Grouped readings and cycling | `features/menu.c`, `features/dashboard_format.c` | Up to four readings per page; optional five-second rotation through the current visible list. |
| Hybrid and Kids pedal modes | `features/pedal_map.c`, `features/pedal_booster.c` | Hybrid follows Natural except Race; Kids uses All Weather with reduced trim and the upstream RPM/speed limiting command. |
| Parking mirror timing | `features/parking_mirrors.c`, `vehicle/mirrors_frames.c` | Delayed return after Drive, brief Neutral transition handling, reverse re-entry cancellation and paced commands. |
| Front PDC mute | `features/parking.c`, `state/parking.c` | Brake-triggered mute outside Reverse; restores only a mute requested by this feature. |
| Reverse audio mute | Same parking modules | Preserves existing mute state, respects manual override and restores audio after leaving Reverse. |
| ACC resume fixes | `vehicle/controls_frames.c`, `vehicle/steering_controls.c` | Recognizes the additional engaged states, restarts only under the required stopped conditions and resets interrupted resume bursts. |
| Window timings | `vehicle/body_commands.c` | Updated close/ajar delays; retains the later gaucho 550 ms ajar interval. |
| AWD reminder | `features/menu.c` | Brief periodic `4WD disabled` reminder without blocking navigation. It reflects the requested mode, not independent actuator feedback. |
| Read trouble codes | `diagnostics/fault_reader.c` | Reads **BCM** faults, including multipart responses, progress, retry and result browsing. This is not an all-ECU fault scanner. |
| Runtime CAN capture | `features/usb_modes.c`, `USB_DEVICE` | Binary USB capture on each board; bounded storage, overflow records and host/disconnect handling. |
| ELM-compatible diagnostics | `protocol/elm327.c`, `transport/diagnostic_link.c` | C1 USB interpreter with controller discovery across C1, C2 and BH, filters, flow control and bounded ISO-TP assembly. |
| IBS override | `features/ibs_override.c`, `features/menu.c` | Explicit experimental runtime action with confirmation; stops when the engine stops or diagnostics starts. Never saved as an enabled preference. |
| Optional UCAN hardware | `platform/system.c`, `platform/power.c` | Optional external 8 MHz oscillator and UCAN power-control pins behind build flags. |

There are **64 gasoline and 60 diesel pages**. Existing page IDs and groups remain
stable; new gasoline IDs are `0x2f..0x40`, and new diesel IDs are `0xb8..0xbc`.
Upstream measurement IDs that collided with this fork's existing readings were
remapped. Redundant source page arrangements are represented by the grouped pages
rather than duplicating measurements. Existing favorites, visibility and sorting
continue to work with the added pages.

All added menu labels and functional comments are in English. The compact
four-temperature page uses `O` = oil, `W` = coolant, `I` = intercooler inlet and
`X` = intercooler outlet, with temperatures in Celsius. The 24-character build
uses `Oil`, `W`, `In` and `Out`. Dedicated single-reading pages retain full labels
such as `Oil temp`, `Coolant temp` and `Battery` with their units.

## Adaptations and exclusions

- Existing fixed-buffer CAN/UART/USB ownership, frame-length checks, display
  coalescing, signed decoding and recoverable records remain in place. Older
  upstream versions of these mechanisms were not substituted for them.
- Board configuration now retries each option when the outgoing queue is full.
  Diagnostic filters exclude unrelated traffic, and remote flow control allows
  at least 10 ms between consecutive CAN frames to accommodate the board link.
- A new diagnostic configuration clears earlier custom flow-control settings.
  Failed configuration sends prevent the dependent request. Receive-buffer loss
  terminates the affected exchange instead of silently combining fragments.
- Optional features default off unless an existing preference/build option already
  enables them. Source authors' personal defaults are not applied to this fork.
- The commented oil-pump and security-access experiments are not working upstream
  menu features. They remain inactive; this integration does not introduce an oil
  pump controller or security-unlock action. The active upstream IBS experiment
  is separately exposed as the explicit temporary action described above.
- Generated CubeIDE output, stale project paths and translated upstream manuals
  are not copied over the current build and English menu documentation.
- Upstream attribution is retained in [LICENSE.MD](../../LICENSE.MD) and adapted
  diagnostic modules.

## Settings and saved data

The existing 40-slot settings record and menu preference record `0x104` are
retained. New options use previously unused settings slots:

| Slot | Menu label | Default |
| --- | --- | --- |
| 31 | `Front PDC mute` | OFF |
| 32 | `Reverse mute` | OFF |
| 33 | `Rotate readings` | OFF |
| 34 | `USB CAN capture` | OFF |
| 35 | `USB ELM327` | OFF |

`Read BCM faults` exposes existing slot 15; its saved value is preserved. Enable
it in Feature setup if the action is absent. Pedal mode slot 20 now accepts values
0 through 8. Maximum hold and IBS override are runtime actions.

Update **C1, C2 and BH together**, using the same display-width configuration on
all three. The diagnostic link extends the board protocol; mixed firmware sets
are not a supported configuration. Saved v5-beta settings and record addresses
are preserved. Compatibility with older pre-refactor saved data has not changed.

## Flash and RAM

**C1 now requires an MCU with physically confirmed 128 KiB Flash.** Its program
exceeds 64 KiB. An ELF file's disk size includes debug data and is not its Flash
requirement, but this particular integration also increases the actual program.
Do not assume a part marked C8 has the extra capacity.

| Region | C1 | C2 / BH / CAN |
| --- | --- | --- |
| Program allocation | 96 KiB, `0x08000000..0x08017fff` | 64 KiB, `0x08000000..0x0800ffff` |
| Disk allocation, when enabled | 20 KiB, `0x08018000..0x0801cfff` | 52 KiB, `0x08010000..0x0801cfff` |
| Recoverable records | `0x0801d000..0x0801ffff` | Same addresses |

C1 uses `STM32F072C8TX_FLASH_C1.ld`; other flavors use the original linker script.
C1's optional disk geometry changes, so an old C1 filesystem image is not reusable.
C2 and BH disk geometry is unchanged. Record access already checks physical Flash
capacity; that runtime check cannot make a C1 program fit a 64 KiB part.

New paths use fixed buffers. The ELM interpreter is included in a normal C1 build
but remains inactive until selected. Normal menu operation performs no dynamic
allocation for the new pages or maximum hold.

## Validation

Local validation uses ARM GCC 15.2.Rel1 and Clang host tests with AddressSanitizer
and UndefinedBehaviorSanitizer. The ten host executables cover:

- existing persistence, SLCAN, CAN/USB ownership and UART behavior;
- menu navigation, settings migration, maximum hold, four-value formatting,
  board-setting retries and both 18/24-character catalog widths;
- BCM single/multipart replies, truncated data, result overflow and timeouts;
- ELM commands, USB busy retry, damaged input and ISO-TP bounds/correlation;
- USB capture overflow, record contents and session expiry;
- parking ownership, freshness, mute restoration, mirror transitions and both
  auxiliary diagnostic bridges.

All four standard flavors compile and pass cppcheck. Additional builds cover
C1 with a large display, MY23, gasoline and LEDs; C2/BH with a large display;
C1 UCAN external clock/power pins; C1 pedal serial adapter; and BH debug mode.
See [build sizes](UPSTREAM_BUILD_SIZES.md) for the measured images and reservations.

These checks validate code and compilation, **not behavior on a connected car**.
Parking timing, actuator behavior, charging response and third-party diagnostic
application compatibility still require hardware checks. No firmware was flashed
or release published by this integration.

For operation, see [USB diagnostics and new menu actions](USB_DIAGNOSTICS.md).
