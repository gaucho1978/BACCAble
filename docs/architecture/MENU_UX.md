# MY23 display menu and extension guide

The catalog contains 64 gasoline pages and 60 diesel pages. Navigation,
preferences and text transport have separate modules. All display labels use
English with selected raw Latin-1 glyphs. Page labels fit within 16 characters, leaving two characters for
editor marks on an 18-character display.

## Engine profile and advanced pages

Features cycles `Engine: 2.0 I4`, `Engine: 2.9 V6`, `Engine: 2.2 D`.
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
| Short RES, then release | Use or confirm the current item; status/reading pages ignore SELECT |
| Hold RES for 1200 ms | Return one level; from the main menu, close |
| Gentle down/up | Next/previous item; hold to repeat in lists |
| Stronger down/up | Next/previous reading, action or setting group |

In favorites, a stronger press also moves one item. Moving through a gentle press
into a stronger press may perform an item step before the group jump. Holding a
gentle direction repeats after 500 ms, then every 180 ms while fresh reports arrive.
Repeat applies to readings, groups, Settings, Features and page editors.
It does not apply to Actions, RES/BACK, stronger presses, or moving a selected
favorite in the reorder editor. Delayed reports never trigger a catch-up burst.
A gap longer than 300 ms in button reports requires a fresh release.
Releasing RES after a hold does not select another item.

Main menu: **Favorites → Readings → Actions → Settings → Information**.
Reading groups: All readings, Engine, Temperatures, Battery, DPF / AdBlue,
Performance, Other. Empty lists show `No pages` or `No favorites`; returning still
works. All browsable lists show their visible position and total. Unavailable but
browsable actions count; hidden entries do not. Drafts, notices, confirmations,
pending/error states and charset ranges are not numbered.

The common layout is `x/y > Label` or `x/y Label: value`. Labels shorten before
values; compact warning rows may omit spacing to retain the entire condition.
Dense readings and long firmware versions use a numbered title for 1200 ms after
selection, followed by their complete original text. Short readings retain their
counter continuously. This width exception avoids dropping values, units or
version characters and does not delay CAN queries or change the 50 ms fragment
interval. It also applies after automatic reading rotation.

## Shared entry types and symbols

`features/ui_entry.h` is the shared contract: toggle, enum, number, action,
conditional action, capture, exclusive mode, submenu and status. Setup descriptors
carry the type and numeric bounds; action descriptors carry the interaction type.
Equivalent types use `ui_render_*` helpers. Vehicle modules retain command and
sequence logic. [Complete entry audit](UNIFIED_UI_AUDIT.md) lists every setup slot,
action, view and side effect.

| Representation | Meaning |
| --- | --- |
| `Ø Auto rotate` / `O Auto rotate` | True editable toggle; SELECT flips the preference |
| `Engine: 2.0 I4` | Named enum; SELECT cycles |
| `* Shift RPM: 3500` | Numeric draft; directions edit, SELECT accepts, BACK cancels |
| `1/8 > BCM faults` | Action/workflow; SELECT enters or requests it |
| `! Start engine` | Known unmet condition or failure |
| `? BH no reply` | Unknown/stale status |
| `< Back` | Exit a submenu |
| `*` in favorite ordering | Selected item being moved |

Production uses ASCII plus the small hardware-reported vocabulary in
`features/ui_glyphs.h`. Editable booleans and membership lists use O/Ø; status-only
and vehicle-request state retain ON/OFF. `*` still means editing/reordering, `>`
means enter/action, `!` is attention/precondition, `?` is unknown, and `×` marks a
failed local operation. A missing vehicle confirmation remains `?`, not a claim
that the requested physical state failed. Signs on numeric readings remain signs.

Temperature formatting reuses an existing unit gap for °C or adds a degree byte
only if all original values/units fit. Dense pages may retain C. Numeric editors
add «/» only when the complete existing label/value still fits; narrow editors
keep their previous format. The verified · and ± are reserved for meaningful
future uses, not added as decoration. See [idle and glyph delivery](IDLE_AND_GLYPHS.md).

`4WD req: OFF WAIT` and `QV req: OPEN WAIT`/`AUTO WAIT` mean an unresolved request,
not measured drivetrain or valve state. 4WD repeats until explicitly cancelled;
its confirmation says `Stop 4WD req? RES`. Exhaust release requests factory control,
not a confirmed closed position. Neither feature invents a positive vehicle ACK.

Queued Dyno/brake commands remain WAIT until their existing C2 reply, a reported
nonmatching result, or a 10-second UI confirmation timeout. Brake replies confirm
an override sequence, not brake pressure. A timeout does not replay or cancel a
vehicle command. Late physical/board behavior may still occur. HAS/clear countdown
completion says `Request sent`, not HAS engaged or faults cleared; ESC has no
reliable measured acknowledgement and can report `No confirmation`.

## Readable measurements and settings

| Page label | Example screen | Meaning |
| --- | --- | --- |
| Oil temp / Oil temp (ECU) | `Oil temp 100°C` | Engine oil temperature |
| Coolant temp | `Coolant temp  90°C` | Engine coolant temperature |
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

Settings describe their state directly: `Engine: 2.2 D`, `Pedal mode: Bypass`,
`Shift RPM: 4500`, `Close: 2 locks`, `Open: OFF`. `Stop block` means suppressing
automatic Start/Stop; `Stop odo blink` means suppressing the blinking odometer.

## Personalization and actions

1. In `Settings → Favorites`, RES adds/removes the selected page. `Ø` marks
   a favorite. Gasoline and diesel each have a six-page limit; I4/V6 share the gasoline list.
2. In `Fav. order`, select an item with RES; `*` marks move mode. Move it
   with the direction controls and press RES again to finish. Movement stops at
   the list boundaries.
3. In `Shown pages`, RES toggles catalog visibility. Hiding a page does not
   remove it from favorites. An automatic performance result may temporarily
   show a hidden page without changing its saved visibility.
4. In `Sort order`, RES switches between functional grouping and A–Z by page
   label. Sorting affects the catalog and editors; favorites retain their custom
   order. Stronger presses move between groups in editors.
5. Return with a long RES. Committed changes are saved automatically when leaving
   configuration or closing the menu. Unchanged domains are skipped; successful
   persistence is silent. There are no explicit Save entries.
   The last favorite and last pages within groups are remembered.

Favorites, visibility and remembered pages are separate for gasoline and diesel;
sort order is shared. Switching engine profiles clears the measurement cache.
`× Save failed: RES` keeps the menu open and the changes in RAM. RES retries the
pending exit explicitly. BACK cancels that exit and stays in the current view;
committed RAM changes remain unsaved until a later successful exit. Other navigation
is ignored while the error is shown; idle processing does not retry automatically.
Persistent failure also prevents closing through the main menu. Settings and menu
preferences are separate saves, not a combined transaction.

Vehicle-control actions require a second RES within three seconds. Moving away
or returning cancels confirmation. Reading BCM faults and toggling maximum hold
do not require that confirmation. Existing availability, stationary-vehicle and dyno
conditions still apply. `Request queued` and WAIT mean that a request was
accepted, not that an ECU confirmed completion. Immobilizer displays its state in Information;
the separate existing steering-wheel gesture changes it only outside the menu;
a long menu direction hold cannot trigger that gesture. `BCM faults` opens
a result browser after the option is enabled in Features. It reads BCM codes,
not faults from every ECU. USB capture, ELM diagnostics and the temporary IBS
action are described in [USB diagnostics](USB_DIAGNOSTICS.md).

`Peak hold` retains numerical maxima until a page/profile change or toggle;
status values remain live. `Auto rotate` advances through the selected list
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

### Fault-read failures

The fault browser keeps the failure reason visible. Press RES to retry or hold
RES to leave. All messages fit the standard and large displays.

| Message | Meaning |
| --- | --- |
| `× Read timeout` | A response or its remaining fragments did not complete within the existing time limits, including repeated pending responses. |
| `× ECU rejected` | The controller returned a negative response other than response-pending. |
| `× Invalid reply` | The reader rejected a response length, payload layout or fragment sequence. |
| `× CAN send failed` | The session request, fault query or flow-control frame was not queued before the read deadline; this does not diagnose a physical CAN fault. |

Unrelated or ignored malformed frames still follow the existing filtering rules
and may ultimately produce a timeout. No new ECU requests or retry rules are added.

### Feedback and inactivity

Simple toggle confirmations last 750 ms; warnings `!` and failure notices `×` last 1800 ms.
Other request notices retain 1200 ms. Navigation dismisses notices immediately;
vehicle confirmation rules are unchanged. Diagnostic progress remains driven by
the diagnostic state, not a cosmetic timer.

After 30 seconds, idle navigation/information returns to Favorites. Settings and
editors allow 60 seconds: unfinished drafts/capture are cancelled and committed
changes are persisted before returning home. The overlay stays visible, and
NEXT/PREV works without reopening it. Favorites and readings remain live; active
fault reading/clearing postpones the automatic return. Only ROOT + BACK explicitly
closes and clears the overlay. A save failure keeps the menu open and requires
deliberate input to retry; it does not repeatedly attempt automatic writes. Existing record storage
skips writing unchanged payloads.

Closing retries the blank screen if UART is busy. Reopening cancels that pending
clear so it cannot erase the new menu. This uses the existing dashboard handover;
physical radio/display behavior still needs vehicle validation.

## Numeric editors, capture and exclusive modes

SELECT enters a numeric draft, NEXT/PREV adjusts by the existing step, SELECT
accepts and BACK cancels without saving the draft. Idle return also discards an
unaccepted draft. Shift RPM: 1500–6000/250; Launch Nm: 25–600/25; Pedal trim:
−10…+10/2. Values clamp at boundaries. Existing in-range saved values are preserved.

Park mirror opens Enabled, Store position and Back. Enabling sends only Enable;
it never captures a position. Enable first, select Store position, adjust the
mirror, then SELECT explicitly confirms capture. BACK cancels. `Store: queued`
means UART accepted the original BH store command; the protocol has no persistence
acknowledgement. Retry rejected sends explicitly. A store never silently enables
a disabled feature.

USB mode cycles OFF → CAN → ELM327 → OFF (without ELM327 support: OFF → CAN → OFF).
The original two persisted flags remain; CAN wins when loading conflicting legacy
flags. The second flag is hidden, including in non-ELM builds, so no independent
switch implies both modes can run together. Stop IBS override before changing USB
mode: USB activation otherwise disables that experiment. Modes apply after successful automatic settings persistence.

Front brake activation explicitly confirms `Brake+launch? RES`, because the
existing C2 reply arms Launch Assist. While launch is active, Front brake refuses
to silently disable it; use the separately named End launch action first.
Known RPM, speed, Dyno and read/clear conflicts are shown before SELECT and checked
again on execution. Dyno confirmation names its ESC reset dependency. Permissions
for active Dyno/brake/4WD/QV/custom ESC cannot be disabled until their operation is
released, avoiding hidden stops or resumed requests when permissions return.

## Hidden IPC diagnostics

Build C1 with `EXTRA_CPPFLAGS=-DMENU_DIAGNOSTICS`; no production menu entry is added
without this flag. Information gains `IPC diag >`. NEXT/PREV cycles raw-byte
groups labelled in hex; SELECT switches to an A/B refresh pattern; BACK returns.
The diagnostic test still covers 0x20–0x7E and 0x80–0xFF as raw single bytes.
The user reports printable ASCII and Latin-1-like 0xA0–0xFF on the tested IPC;
0x80–0x9F is unsupported/control and must not be used by production renderers.
Only the selected constants in ui_glyphs.h are adopted. Diagnostic coverage is not
approval of every candidate on every IPC revision. MY23 and LARGE_DISPLAY remain
independent settings; validate other physical IPC variants before assuming a match.


## Global interaction contract

NEXT/PREV moves between peers or changes the current numeric draft. SELECT uses
or confirms the current item. BACK cancels an unfinished workflow or returns one
level. Committed configuration changes save automatically, silently on success;
a failed exit remains explicit and retryable. `x/y` is the position within the
currently browsable list, subject to the dense-data title exception above.

| View | SELECT | BACK |
| --- | --- | --- |
| ROOT | Enter selected section | Close |
| FAVORITES | No-op | ROOT |
| GROUPS | Enter readings | ROOT |
| VALUES | No-op | GROUPS |
| ACTIONS | Run/confirm selected operation | ROOT |
| SETTINGS | Enter editor or change sort | ROOT |
| FEATURES | Use entry / accept numeric draft | Cancel nested draft/capture, otherwise SETTINGS |
| EDIT FAVORITES / SHOWN PAGES | Toggle membership/visibility | SETTINGS |
| FAVORITE ORDER | Pick/drop a nonempty item | SETTINGS |
| INFORMATION | No-op; explicit IPC diagnostic entry opens its screen | ROOT |
| FAULTS | Restart read if Clear is inactive | Cancel read, ACTIONS |
| DIAGNOSTICS | Switch test pattern | INFORMATION |

Park Mirror has its own Enabled / Store position / Back list. BACK from capture
returns to that list; BACK again returns to Features. The explicitly labelled
`< Back` row is the sole SELECT-as-return entry. Physical BACK while the menu is
closed still opens Favorites; the physical button protocol is unchanged.

[Delivery and acceptance audit](../MENU_UX_CONSISTENCY_DELIVERY.md) records the
requirements, preserved behavior and hardware validation still needed.
