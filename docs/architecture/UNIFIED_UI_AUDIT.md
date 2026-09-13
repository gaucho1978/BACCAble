# Unified menu interaction audit

Historical beta-6 inventory. For current navigation, automatic persistence and
numbering, see [the consistency delivery](../MENU_UX_CONSISTENCY_DELIVERY.md) and
[the menu guide](MENU_UX.md).

This inventory accompanies [the UX requirements](../BACCAble%20Unified%20Menu%20UX%20and%20Interaction%20Refactor%20Plan.md). Firmware sources, rather than vehicle assumptions, determine what can be reported as confirmed. Implementation and host validation are recorded below; hardware validation remains separate.

## Shared contract

`features/ui_entry.h` defines toggle, enum, number, action, conditional action, capture, exclusive mode, submenu and status. Descriptors remain static; vehicle modules retain CAN generation, sequencing and feedback interpretation. `features/ui_entry.h` also owns production symbols. Use ASCII `>` for entry/action, `!` for unavailable/failure, `?` for unknown, `*` for editing, and explicit ON/OFF for booleans. WAIT means unresolved work, not physical success. No extended IPC glyph is approved by this audit. The historical 0xD8 marker does not establish any general charset mapping.

NEXT/PREV navigates entries; inside numeric editors it changes a value. SELECT follows the entry type; BACK returns to the parent or cancels an unaccepted editor. Existing long RES hold maps to BACK; release maps to SELECT. No new double-click or hold duration is introduced. Navigation repeats only in contexts allowed by `menu_button`; input loss disarms the gesture. Settings apply in RAM and save on explicit Save or exiting setup/root; save failures remain retryable. Saving also restarts board setting synchronization and applies USB personality.

In the tables, **nav** means NEXT/PREV chooses the adjacent entry (group gestures choose groups where supported). **T** means a local preference toggle, SELECT flips it, nav selects another entry, no asynchronous confirmation, ON/OFF symbol; availability is the C1 feature setup unless stated. A feature permission enables access/processing, not proof that the corresponding vehicle feature is physically active.

## Every persisted SetupParam

Slots are the persistence identity, not display order. All slots 1–37 are accounted for; slot 35 remains hidden in every build to preserve its stored value. Hidden members remain persisted. Active Dyno/brake/4WD/QV/custom ESC permissions reject disable requests; USB selection rejects changes while IBS override is enabled. Storage widths and IDs remain authoritative in `settings/setup_entries.c` and `settings/setup_menu.h`.

| Slot | Label / stored member | UI type and SELECT | NEXT/PREV | Availability; pending / effect | Symbol |
|---|---|---|---|---|---|
| 1 | Immobilizer / `immobilizer_enabled` | Hidden toggle; no menu SELECT | None | Existing deliberate vehicle gesture; see below | Information ON/OFF |
| 2 | Stop block | T | nav | Clears this feature's old stop-block request | ON/OFF |
| 3 | LED strip | T | nav | LED feature preference | ON/OFF |
| 4 | Shift light | T | nav | Shift notification preference | ON/OFF |
| 5 | Shift RPM | Number; enter/accept editor | ±250, clamp 1500–6000 | Shift threshold, no vehicle ACK | `*` editing |
| 6 | MY23 display | T | nav | IPC generation independent of 18/24-character width | ON/OFF |
| 7 | Route msgs | T | nav | Diagnostic routing preference | ON/OFF |
| 8 | Allow Dyno | T | nav | Action permission | ON/OFF |
| 9 | ACC pad | T | nav | Virtual control preference | ON/OFF |
| 10 | Allow brake | T | nav | Front brake action permission | ON/OFF |
| 11 | Allow 4WD | T | nav | Drivetrain request processing permission | ON/OFF |
| 12 | `remote_start_enabled` | Hidden toggle | None | Persisted compatibility option | None |
| 13 | Allow clear | T | nav | Clear action permission | ON/OFF |
| 14 | Allow ESC/TC | Toggle with board command | nav | Disabling while custom stability behavior is active is rejected; release first | ON/OFF, `!` |
| 15 | Allow read | T | nav | Read action permission | ON/OFF |
| 16 | Engine / `is_diesel_enabled` | Enum: 2.2 D, 2.0 I4, 2.9 V6 | nav | Coordinates slot 36, updates page catalog; stable page IDs retained | Named value |
| 17 | DPF alert | T | nav | Alert preference | ON/OFF |
| 18 | Launch Nm | Number; enter/accept editor | ±25, clamp 25–600 | Launch torque threshold, not launch enable | `*` editing |
| 19 | Belt alarm | T | nav | Alarm preference | ON/OFF |
| 20 | Pedal mode | Enum: OFF, Auto, Bypass, A, N, D, R, Hybrid, Kids | nav | Sends board mode; OFF selects default map | Named value |
| 21 | Stop odo blink | Toggle with board command | nav | Sends BH default/disable command; local preference, no physical ACK | ON/OFF |
| 22 | `show_race_mask` | Hidden toggle | None | Optional custom ESC display behavior | None |
| 23 | Park mirror | Submenu: enabled toggle and explicit position capture | nav; BACK cancels capture | Store command also enables BH function; never issue store from generic toggle; no capture ACK | `>`, ON/OFF, `!` |
| 24 | ACC resume | Enum: OFF, RES, + | nav | Existing resume gesture mode | Named value |
| 25 | Close windows | Enum: OFF, 1 lock, 2 locks | nav | Clears pending close gesture and lock counter | Named gesture |
| 26 | Open windows | Enum: OFF, 1 unlock, 2 unlocks | nav | Clears pending open gesture and unlock counter | Named gesture |
| 27 | Allow HAS | Toggle with board command | nav | Sends C2/BH permission; no HAS engagement ACK | ON/OFF |
| 28 | Allow exhaust | T | nav | Exhaust request processing permission | ON/OFF |
| 29 | Pedal trim | Number; enter/accept editor | ±2, clamp −10…+10 | Invalidates cached map indication on accepted change; stored signed byte | `*` editing |
| 30 | `eujot_enabled` | Hidden toggle | None | Persisted compatibility option | None |
| 31 | PDC mute | T | nav | Parking mute preference | ON/OFF |
| 32 | Reverse mute | T | nav | Reverse audio preference | ON/OFF |
| 33 | Auto rotate | T | nav | Enables automatic reading rotation | ON/OFF |
| 34 | USB mode / `usb_sniffer` | Exclusive mode: OFF, CAN, ELM327 when built | nav | Coordinates slot 35; selected personality takes effect on save | Named mode |
| 35 | `usb_elm327` | Hidden companion of USB mode | None | ELM327 offered only with `ACT_AS_ELM327`; hidden flag retained in every build | None |
| 36 | `gasoline_v6` | Hidden companion of Engine enum | None | Distinguishes I4/V6 with slot 16 | None |
| 37 | Advanced | T | nav | Reveals advanced telemetry pages | ON/OFF |

Pedal A/N/D/R retain the existing letter-named response maps; Auto follows drive mode. Consult the vehicle drive-mode naming rather than treating a letter as a new curve. This refactor does not change their curves. Hidden storage members do not gain user-facing controls merely because they have a semantic classification.

## Actions and feedback provenance

Every action is listed below. NEXT/PREV always navigates actions. Feature permissions filter their visibility; known temporary conditions should render before SELECT using the same availability function that gates execution. Confirmation, if required by the menu, is an additional SELECT, never a second interaction type. Requests rejected by a full queue must not report success.

| Label | Type; SELECT | Availability before execution | Pending, confirmation and side effects | Symbol |
|---|---|---|---|---|
| QV exhaust | Conditional action; OPEN/AUTO request | Allow exhaust | 1 starts, 2/3 repeat OPEN request, 4 requests AUTO then 0; Chinese remote pulse also requested. Neither proves valve position | `>`, req, WAIT |
| HAS button | Action; inject virtual press | Allow HAS | C1/C2 countdown of five frames; no engagement state acknowledgement | `>`, WAIT |
| ESC/TC | Conditional action; request custom behavior change | Allow ESC/TC; C2 rejects during Dyno or its sequence | C1 display inversion may follow optional race-mask broadcast; not measured ESC/TC state | `>`, req, `!` |
| Dyno | Conditional action; toggle request | Allow Dyno; steady counter ≥100; no active brake; custom ESC dependency | C2 state machine queries ABS and writes mode; requesting Dyno clears C2 custom ESC state | `>`, WAIT, `!` |
| Front brake | Conditional action; force/release request | Allow brake; engaging requires speed=0 and Dyno; launch must be released first; activating confirms Brake+launch | C2 reports start/release sequence; force ACK enables Launch Assist on C1. No brake-pressure feedback | `>`, req, WAIT, `!` |
| Release launch | Conditional action; explicitly disable local Launch Assist | Allow brake and launch active | Confirmed separate action; leaves front brake override unchanged | `>`, `!` |
| 4WD | Conditional action; start/cancel request sequence | Allow 4WD; starting requires steady counter ≥100 | Sequence 4→3→2→1, then 1 repeats indefinitely. Cancel stops requests; does not prove 4WD ON | `>`, req, WAIT, `?` |
| Read BCM faults | Conditional action/workflow; read then browse | Allow read; Clear inactive | UDS reader distinguishes progress/results/rejected/timeout/invalid response/send failure; BACK cancels | `>`, WAIT, `!` |
| Clear faults | Conditional action/workflow; request sweep | Allow clear; Read inactive; no duplicate clear | Count 255→0 reflects accepted local CAN sends, not ECU positive clear acknowledgements; peer clear completion unknown | `>`, WAIT, `!` |
| Reset records | Action; erase saved acceleration records | Always | Synchronous flash result; only reports cleared after save succeeds | `>`, `!` failure |
| Maximum hold | Toggle; flip peak hold | Always | Local diagnostic value presentation; no vehicle command | ON/OFF |
| IBS override | Conditional toggle | RPM >400 (existing threshold) | Local override enable; engine-off/vehicle logic governs lifetime; not a measured battery state | ON/OFF, `!` |

`app/board_commands.c` receives `C1cmdForceFrontBrake`, `C1cmdNormalFrontBrake`, `C1cmdDynoActive` and `C1cmdDynoNotActive`. Brake replies are generated by `features/chassis.c` before/while sending override frames. Dyno replies follow ABS diagnostic responses in `vehicle/diagnostic_frames.c`, but a negative response or four-second C2 timeout also reports its retained mode. A C1 notification is consequently a board-reported mode, not a universally distinct success/failure code.

`vehicle/steering_controls.c` decrements HAS requests while observing button traffic; this is transmission progress, not a HAS-state signal. `features/drivetrain.c` and `features/exhaust.c` have no positive vehicle acknowledgement. `chinese_valve_is_opened` is assigned from the GPIO request in `exhaust.c`; its name must not be treated as evidence of sensing. Clear's local enqueue sweep in `app/main.c` does not parse per-ECU clear results. The read workflow does parse UDS responses and can legitimately report its read results.

## All views, editors and reading entries

| View / labels | Type; SELECT | NEXT/PREV | Availability; pending / effect | Symbol |
|---|---|---|---|---|
| ROOT: Favorites, Readings, Actions, Settings, Information | Submenus; enter | nav | Menu allowed with CC/ACC off | `>` |
| GROUPS: All readings, Engine, Temperatures, Battery, DPF / AdBlue, Performance, Other | Submenus; open group | nav | Engine/page capability filter | `>` |
| FAVORITES / VALUES: each catalog label and stable ID below | Status; SELECT returns root | Page navigation; group jumps for VALUES | Visible, supported profile pages; queries paused during Clear; automatic rotation optional | `?` / reading quality markers |
| FUNCTIONS | Types in action inventory | nav/group navigation | Per-action permission and condition | Shared action symbols |
| SETTINGS: Feature setup, Edit favorites, Visible pages, Order favorites | Submenus; enter | nav | C1 menu | `>` |
| SETTINGS: Sort order | Enum; groups ↔ A–Z | nav | Changes listing order only | Named value |
| SETTINGS: Save; SETUP save/return page | Action; persist settings/preferences | nav | Save failure permits retry | `>`, `!` |
| SETUP: every visible slot above | Descriptor-defined type | nav/group navigation, numeric edit when active | Hidden slots omitted | Descriptor-defined |
| EDIT_FAVORITES: each supported catalog label/ID | Toggle; favorite membership | nav/group navigation | Maximum six; overflow notice; independent of visibility | ON/OFF |
| EDIT_VISIBLE: each supported catalog label/ID | Toggle; page visibility | nav/group navigation | Changes preference, not measurements | ON/OFF |
| ORDER_FAVORITES: favorite catalog labels/IDs | Editor; pick/drop selected favorite | Navigate or move selected favorite | Saves on BACK; stable identity preserved | `*` selected |
| FAULTS: result, no faults, progress, failure | Status/workflow; SELECT restarts read when allowed | Browse fault result index | Read/Clear exclusion; BACK cancels | WAIT, `!` |
| INFO: firmware version; C2 version; BH version; MY23/width; Immobilizer | Status; SELECT returns root | nav | Peer version becomes unknown after five seconds without reply | `?`, ON/OFF |
| Hidden IPC charset screen | Diagnostic status; switch charset/display pattern mode | Cycle byte groups or display pattern | `MENU_DIAGNOSTICS` build only; 0x20–0x7E and 0x80–0xFF, hex labels, raw bytes, never UTF-8 | Byte hex labels |
| Numeric editor | Number; accept | Increment/decrement within bounds | BACK discards unaccepted value | `*` |
| Park mirror capture prompt | Capture; explicitly confirm store | Cancel/navigate according to workflow | BACK never stores; queue acceptance is not saved-position ACK | `>`, `!` |

The complete per-reading inventory is `parameter_pages` in [parameter_catalog.c](../../firmware/baccable/diagnostics/parameter_catalog.c): each `.id` and `.label` is an individual STATUS entry in FAVORITES/VALUES, a membership toggle in EDIT_FAVORITES/EDIT_VISIBLE, and a movable entry in ORDER_FAVORITES. These equivalent pages share the table's complete interaction rules; they do not each implement SELECT callbacks. `menu_model.c` filters engine capability, advanced-page visibility and stable IDs. Parameter enum values (gear, DNA, regen state, seatbelt state and statistics state) are measured/status renderers, not selectable enum settings. Invalid/offline/stale values must retain quality markers instead of becoming zero or a claimed physical state.

## Search outside the menu

The audit searched all `firmware` files for labels, setup descriptors, toggle/cycle callbacks, SELECT handlers, state capture, request flags and rendering. Vendor HAL callbacks are hardware plumbing, not menu entries. Additional product paths were checked:

* `features/dashboard.c`, `display_stream.c`, `body.c` and `vehicle/display_frames.c`: byte-oriented presentation and dashboard ownership; no independent menu SELECT. Fixed-width buffers, not UTF-8, feed both 18- and 24-byte builds.
* `parameter_navigation.c`, `diagnostics/parameter_request.c`, `parameter_cache.c`, `native_parameters.c`: automatic result selection, request/cache status and measurement formatting; no editable measurement values.
* `vehicle/controls_frames.c`, `drive_mode.c`, `steering_controls.c`: pre-existing physical stalk, gear-release, park-assist and drive-mode gestures for ESC, HAS, Dyno, QV and immobilizer. These remain vehicle controls, not a newly invented menu gesture language.
* Outside the menu, the legacy immobilizer gesture holds a cruise button for roughly 30 seconds with engine running, neutral and CC/ACC disabled, saves immediately and reports with dashboard blinks. The menu now resets/disarms that legacy gesture while it owns the controls.
* `features/parking_mirrors.c` / mirror frame handlers and BH command dispatch: actual position capture remains a vehicle module operation. The Store command also enables BH Park Mirror; UI must explicitly account for that dependency and avoid claiming a position acknowledgement it never receives.
* `settings/persistence.c`, board synchronization, USB mode modules and board commands: persistence and remote application remain separate from local UI state. USB mode resolves conflicting flags while preserving old storage slots.
* Parking/audio and automatic start/stop/window handling consume their own preferences. Clearing a feature's own pending gesture on preference change prevents stale execution; this is distinct from silently changing another visible feature.

## Acceptance evidence and hardware gates

The shared contract is implemented and covered by host regressions, including both normal display widths and diagnostic-menu integration. Physical IPC glyph verification, both display widths on actual clusters, fast navigation and in-vehicle unavailable/pending workflows remain hardware acceptance gates. Extended glyphs must stay out of production until their exact raw bytes are verified on the physical IPC.

## Requirements traceability

| Original sections | Implementation / evidence |
| --- | --- |
| 1–2, 4–9, 25, 27–28, 38 | Static shared types, symbols and render helpers; setup type/range metadata and action type descriptors. Typed numeric/capture/exclusive workflows; no allocations, new RTOS or vehicle-logic relocation. |
| 3, 30, 35–36 | MENU_DIAGNOSTICS-only raw-byte charset and A/B display test, hex range labels, complete requested byte coverage and bounded-buffer tests. Production stays ASCII. Extra diagnostic counter pages are optional examples, not invented signals. |
| 10 | Park enable does not capture. Explicit enabled-only Store confirmation, cancel/retry and exact BH opcode tests; queued status does not imply persistence ACK. |
| 11–18 | ACC/window/pedal/engine enums, three clamped numeric editors and one USB mode. Tests cover cycles, ranges, draft accept/cancel and original storage slots. |
| 19–23 | Shared upfront/execution guards, explicit launch release, 4WD cancel, QV request-only state, RPM guard, read/clear exclusion, existing detailed fault failures. Pending tests cover replies, failure, timeout and cancel; physical success is never fabricated. |
| 24, 26, 34 | Complete slot/action/view inventory above, firmware-wide search, active-permission guards and menu ownership of the legacy immobilizer gesture. |
| 29, 31–33 | 18/24 host builds, diagnostic integration, unchanged page IDs/Favorites and stored flag encoding, new per-scenario regressions. |
| 37 | Shared contract and examples in MENU_UX.md and this audit. No non-ASCII production approval without physical tests. |
| 39 | Foundations/settings/workflows/audit implemented in one branch; physical IPC phase remains a hardware check. |
| 40 | Software acceptance covered by the inventory and tests. Physical cluster glyphs, navigation feel, vehicle responses and both real display widths remain unverified here. |

`Release launch` is an intentional added action; legacy gesture suppression while
the menu is open and rejecting permission-off for active operations are intentional
safety changes. Setup labels are shortened where needed to retain complete ON/OFF
values on the 18-character screen. Underlying sequence logic and CAN payloads are
unchanged, apart from moving mirror Store to explicit confirmation and observing
existing C2 replies for UI feedback.

Local validation: 110 reported scenarios passed across 13 host executables with
ASan/UBSan (normal 18/24 menu, hidden-menu integration and raw charset bounds).
Four production firmware flavors and the LARGE_DISPLAY + MENU_DIAGNOSTICS C1
build are checked separately; CI repeats production builds and size gates.
