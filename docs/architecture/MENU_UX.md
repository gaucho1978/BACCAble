# MY23 display menu and extension guide

The catalog contains 64 gasoline pages and 60 diesel pages. Navigation,
preferences and text transport have separate modules. All display labels use
English ASCII. Page labels fit within 16 characters, leaving two characters for
editor marks on an 18-character display.

## Engine profile and advanced pages

Feature setup cycles `Engine: 2.0 I4`, `Engine: 2.9 V6`, `Engine: 2.2 D`.
Existing gasoline settings default to I4; V6 owners should select V6 once.
`Advanced pages` reveals technical and secondary layouts. Favorites retain saved
IDs and can include advanced pages, but incompatible engine pages are temporarily
filtered. Setup `Allow ...` switches permit access to Actions; they do not execute
those actions. Immobilizer status is in Information.
See the [full catalog audit](CATALOG_AUDIT.md) for classification and migration.

## Controls

Menu controls are available when both cruise control and adaptive cruise control
are disabled. Release the buttons after disabling them. Hold RES for 1200 ms to
open the last favorite. The distance button has the same menu function as RES.

| Gesture | Result |
| --- | --- |
| Short RES, then release | Enter or select; from a reading, open the main menu |
| Hold RES for 1200 ms | Return one level; from the main menu, save and close |
| Gentle down/up | Next/previous item, once per press |
| Stronger down/up | Next/previous reading, action or setting group |

In favorites, a stronger press also moves one item. Moving through a gentle press
into a stronger press may perform an item step before the group jump. There is no
autorepeat. A gap longer than 300 ms in button reports requires a fresh release.
Releasing RES after a hold does not select another item.

Main menu: **Favorites → Readings → Actions → Settings → Information**.
Reading groups: All readings, Engine, Temperatures, Battery, DPF / AdBlue,
Performance, Other. Empty lists show `No pages` or `No favorites`; returning still
works. The main menu and group list show position counters; editors use full labels.

## Menu symbols

| Symbol | Meaning | Example |
| --- | --- | --- |
| `+` | Enabled state or included page | `+ Maximum hold` |
| `-` | Disabled state or excluded page | `- Maximum hold` |
| `*` | Selected item for moving or current sort mode | `* Sort: A-Z` |
| `>` | Opens a submenu or browser | `> Feature setup` |
| `<` | Saves and returns to the parent menu | `< Save and back` |
| `!` | Warning, failed action or confirmation required | `! RES to confirm` |
| `?` | Unknown board status | `? BH no reply` |

Requested vehicle changes retain `Req ON` / `Req OFF`; these are not confirmed
states. Reading values retain their full width and existing missing-value `--`
notation. Hold RES to return from other screens; the symbols do not add buttons.

## Readable measurements and settings

| Page label | Example screen | Meaning |
| --- | --- | --- |
| Oil temp / Oil temp (ECU) | `Oil temp 100 C` | Engine oil temperature |
| Coolant temp | `Coolant temp  90C` | Engine coolant temperature |
| Battery voltage | `Battery 14.20 V` | Battery voltage |
| Battery current | `Battery  -12.3 A` | Signed battery current |
| Battery charge | `Batt charge  80%` | Reported battery state of charge |
| Oil pressure | `Oil press  1.20bar` | Engine oil pressure |
| DPF temp | `DPF temp  650 C` | Particulate-filter temperature |

Voltage, current and state of charge remain separate readings; voltage alone is
not presented as proof that the alternator is charging. Dual readings include
separate units, for example `Batt 14.2V  -12.3A`. Fields also accommodate
`-150.5 A` and negative boost pressure without removing the measurement sign.
Temperatures and speed use whole units; a single voltage reading uses two decimal
places and current uses one. This changes presentation rounding, not decoding.
All 124 page templates, including units, are checked in both 18- and 24-character builds.
Performance states `MISS` and `RUN` do not receive a seconds suffix.

Settings describe their state directly: `Engine: 2.2 D`, `Pedal: Bypass`,
`Shift at 4500 RPM`, `Close: 2 locks`, `Open windows OFF`. A leading `+` enables
the named boolean behavior; `-` disables it. `Auto stop block` means suppressing
automatic Start/Stop; `Stop odo blink` means suppressing the blinking odometer.

## Personalization and actions

1. In `Settings → Edit favorites`, RES adds/removes the selected page. `+` marks
   a favorite. Gasoline and diesel each have a six-page limit; I4/V6 share the gasoline list.
2. In `Order favorites`, select an item with RES; `*` marks move mode. Move it
   with the direction controls and press RES again to finish. Movement stops at
   the list boundaries.
3. In `Visible pages`, RES toggles catalog visibility. Hiding a page does not
   remove it from favorites. An automatic performance result may temporarily
   show a hidden page without changing its saved visibility.
4. In `Sort order`, RES switches between functional grouping and A–Z by page
   label. Sorting affects the catalog and editors; favorites retain their custom
   order. Stronger presses move between groups in editors.
5. Use `Save` or return with a long RES. Leaving an editor saves preferences;
   leaving feature options also saves settings. Closing the main menu saves both.
   The last favorite and last pages within groups are remembered.

Favorites, visibility and remembered pages are separate for gasoline and diesel;
sort order is shared. Switching engine profiles clears the measurement cache.
`Save failed: RES` keeps the menu open and the changes in RAM. RES retries the
save or pending return instead of accidentally toggling the selected option.
Persistent failure also prevents closing through the main menu. Settings and menu
preferences are separate saves, not a combined transaction.

Vehicle-control actions require a second RES within three seconds. Moving away
or returning cancels confirmation. Reading BCM faults and toggling maximum hold
do not require that confirmation. Existing availability, stationary-vehicle and dyno
conditions still apply. `Command queued` and `requested` mean that a request was
accepted, not that an ECU confirmed completion. Immobilizer displays its state in Information;
the separate existing steering-wheel gesture changes it. `Read BCM faults` opens
a result browser after the option is enabled in Feature setup. It reads BCM codes,
not faults from every ECU. USB capture, ELM diagnostics and the temporary IBS
action are described in [USB diagnostics](USB_DIAGNOSTICS.md).

`Maximum hold` retains numerical maxima until a page/profile change or toggle;
status values remain live. `Rotate readings` advances through the selected list
every five seconds. Dedicated single-value pages remain available for clearer labels.

## Responsiveness and memory

- Button events use elapsed time and transitions, without blocking menu delays.
- Settings share one current-screen buffer: 18 or 24 bytes instead of 40 page
  buffers. `void render(void)` callbacks write `dashboard_setup_screen`.
  Integer settings avoid float formatting.
- Lists are sorted when built. Only the current view is formatted; menu storage
  has fixed capacity and does not allocate heap memory.
- Periodic rendering runs every 100 ms. Identical text is suppressed for 500 ms,
  then resent to keep the display active.
- UART retains the latest waiting screen, preserves command FIFO and never
  overwrites an active transfer. A screen may precede a waiting status poll only
  once, so continuous browsing does not starve status replies.
- BH prioritizes the latest screen and skips unchanged fragments. CAN queue
  rejection keeps work pending for retry. Factory text requests a full refresh
  without resetting progress through the fragments.
- Native readings refresh only from their corresponding valid CAN frames. UDS
  issues at most one request every 500 ms, after a 150 ms page-settling interval,
  cycling through up to four page values. Fault clearing pauses polling; the fault
  browser uses a separate transaction. Replies must match ECU, DID, profile and page.
- Readings older than three seconds show `--`. Local performance records and
  free-memory readings do not expire. Values wider than their field also show
  `--` instead of a truncated number.

The 100 ms render interval is not a measured dashboard response time. UART still
requires a gap greater than 250 ms; display text travels in three-character parts
at intervals of at least 50 ms: six parts for 18 characters, eight for 24. Fast
browsing can skip intermediate waiting screens. Actual smoothness, factory-message
interaction and Race mask behavior still require vehicle testing.

BH replaces unsent content with the latest target and sends only changed
three-character fragments. Round-robin selection prevents frequent changes from
starving the end of the screen; rejected CAN submissions remain pending. Nonblank
text is refreshed after factory display traffic and after 500 ms without a
successful fragment submission. CAN acceptance does not confirm IPC rendering:
fragment updates cannot guarantee an atomic screen change.

## Compatibility

`LARGE_DISPLAY` selects 24 characters; otherwise the width is 18. C1, C2 and BH
must use matching widths because UART message length also changes.
`IPC_MY23_IS_INSTALLED` does not replace `LARGE_DISPLAY`. Information shows C1
version, C2/BH replies and MY23/width settings. A board silent for more than five
seconds shows `no reply`; older firmware without status replies can do the same.

Menu preferences use an explicitly serialized 80-byte record, type `0x104`, version
1, in the existing visibility slot. Without that record, the menu imports the
previous `0x103` visibility record by historical indices. Saving replaces it with
the new format. Settings, performance and mirrors retain their own records. This
migration covers the previous branch format, not arbitrary original 3.1.1 data.
Storage requires the physical Flash capacity described in [architecture](README.md).

## Adding a reading or function

1. Add or reuse a `ParameterDefinition` in `diagnostics/parameter_catalog.c`.
   For a new native value, add its read in `native_parameters.c` and its actual
   incoming-frame cache update in `parameter_cache.c`.
2. Add a `ParameterPage` with `id`, `group`, English `label`, `name` template,
   `parameter_ids` and `element_count` (1–4; zero keeps the legacy count of two).
   Labels fit 16 ASCII characters; expanded templates fit the selected 18/24 width.
   Preserve existing page IDs and table order. IDs currently fit gasoline
   `0x01..0x40` and diesel `0x81..0xc0`; never reuse an ID for a different reading.
3. Check capacity before appending: the gasoline catalog fills all 64 slots;
   diesel uses 60. A 65th page requires catalog/list expansion, a new visibility
   representation and saved-preference migration. Update counts and tests together.
   Group 0 is All readings; assign pages to groups 1–6.
4. Add actions to the enum and `actions` table in `features/menu.c`. Define
   availability, execution conditions, command and status presentation. Put the
   device behavior in the appropriate feature module. For configurable values,
   extend `SetupParam` and keep persisted setting slots stable.
5. Run host tests, lint and the relevant firmware builds. Check linker sizes after
   adding text or features: C1 has a 96 KiB program allocation and needs physical
   128 KiB Flash. Other flavors retain 64 KiB program allocations.

## Validation

See the [integration report](UPSTREAM_SYNC.md) and
[dated build measurements](UPSTREAM_BUILD_SIZES.md) for validation scope and sizes. Host tests use
production code with HAL/storage substitutes and cover gestures, lost reports,
clock wrap, complete and partial display transfers, target replacement, fair fragment
selection, retries, command ordering, sorting,
favorites, migration, empty lists, remembered pages, save failure, profile changes,
late UDS replies, expiry, label/template widths, negative current and setting buffers.

```sh
make -C tests test
make -C firmware/baccable FLAVOR=C1 lint
make -C firmware/baccable -j4 FLAVOR=C1 VERSION=local-test
make -C firmware/baccable -j4 FLAVOR=C1 BUILD_DIR=build/C1-menu-large \
  VERSION=local-test EXTRA_CPPFLAGS="-DLARGE_DISPLAY -DIPC_MY23_IS_INSTALLED -DIS_GASOLINE -DLED_STRIP_CONTROLLER_ENABLED"
```

Repeat baseline builds and lint for C2/BH/CAN. A 24-character board set also needs
C2 and BH built with `-DLARGE_DISPLAY`; the MY23 option is handled on C1. Set `TOOLCHAIN`
to the ARM compiler prefix if it is outside PATH. Host results do not establish
physical-device or remote-CI results for these changes.
