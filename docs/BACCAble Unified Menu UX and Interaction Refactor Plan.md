# BACCAble Unified Menu UX and Interaction Refactor Plan

## Purpose

Refactor the BACCAble menu and setup UX so that all settings, actions, values and workflows use a small set of consistent interaction types and a shared visual language.

The current firmware works, but several menu entries have hidden semantics:

- some entries are true toggles,
- some cycle through enums,
- some are numeric values,
- some execute actions,
- some execute actions only when conditions are met,
- some modify another feature as a side effect,
- some capture current vehicle state while appearing to be a simple toggle,
- some options are mutually exclusive but are presented as independent switches.

This creates avoidable ambiguity.

The goal is to make the menu self-explanatory enough that a user can understand what an entry does from its presentation and behavior without needing prior knowledge of the implementation.

Do not perform a broad architectural rewrite. Reuse the current menu, setup, persistence and state architecture where practical.

---

# Primary Goal

Introduce a **shared UI entry type system** and **shared symbol vocabulary**, then audit the entire firmware and migrate all relevant menu/setup entries to that model.

The result should make future features easier to add consistently.

The developer adding a new feature should be able to decide:

```text
This is a toggle.
This is an enum.
This is a numeric value.
This is an action.
This is a conditional action.
This is a capture/setup operation.
This is an exclusive mode.
```

and automatically get predictable rendering and interaction behavior.

---

# Important Existing Areas

Review at minimum:

```text
firmware/baccable/features/menu.c
firmware/baccable/features/menu_input.c
firmware/baccable/settings/setup_entries.c
firmware/baccable/settings/setup_entries.h
firmware/baccable/settings/persistence.c
firmware/baccable/state/*
firmware/baccable/features/display_stream.c
firmware/baccable/features/body.c
firmware/baccable/diagnostics/fault_reader.c
tests/
```

Also search the entire firmware for:

```text
menu labels
setup definitions
cycle callbacks
toggle callbacks
value renderers
action status strings
availability checks
side effects caused by SELECT
feature enable flags
```

Do not limit the audit only to the examples in this document.

---

# 1. Introduce Shared UI Entry Types

Implement a lightweight common type system.

A possible model is:

```c
typedef enum {
    UI_ENTRY_TOGGLE,
    UI_ENTRY_ENUM,
    UI_ENTRY_NUMBER,
    UI_ENTRY_ACTION,
    UI_ENTRY_CONDITIONAL_ACTION,
    UI_ENTRY_CAPTURE,
    UI_ENTRY_EXCLUSIVE_MODE,
    UI_ENTRY_SUBMENU,
    UI_ENTRY_STATUS
} UiEntryType;
```

The exact implementation may differ if the existing setup architecture supports a simpler solution.

The important requirement is semantic consistency, not this exact enum.

Each entry should expose enough metadata for the UI to understand:

```text
label
entry type
current value/state
availability
optional unavailable reason
optional pending state
optional action/callback
optional range / step
optional enum values
```

Avoid creating an oversized generic UI framework.

Keep it simple and static.

---

# 2. Define a Shared Symbol Vocabulary

Centralize display symbols in one header/module instead of embedding arbitrary characters throughout menu code.

Start with safe ASCII symbols and add dashboard-specific raw byte glyphs only after hardware verification.

Suggested semantic vocabulary:

```text
>   enter / submenu / action
!   warning / unavailable / attention
?   unknown / no data / offline
*   selected/editing only if needed
```

For boolean state, prefer a dedicated checkbox-style glyph pair if verified by the IPC charset.

The original BACCAble firmware uses:

```text
0x4F = O
0xD8 = Ø
```

as unselected/selected markers.

Do not assume the entire IPC charset is Latin-1 or CP1252 solely because `0xD8` renders correctly.

---

# 3. Add an IPC Character Set Test

Create a hidden diagnostic/test screen capable of displaying raw byte values.

Test at least:

```text
0x20–0x7E
0x80–0xFF
```

The goal is to determine the actual dashboard glyph map.

Do not encode test characters using UTF-8.

Send raw single-byte values because the current dashboard transport is byte-oriented.

Useful candidates to verify include:

```text
0xAB  «
0xBB  »
0xB0  °
0xB1  ±
0xB7  ·
0xD7  ×
0xD8  Ø
```

Also test CP1252-only candidates such as:

```text
0x85  …
0x95  •
0x8B  ‹
0x9B  ›
```

but do not use them in production UI until confirmed on physical IPC hardware.

Provide a simple way to cycle through character ranges and identify bytes visually.

The final production symbol table must contain only confirmed glyphs.

---

# 4. Boolean Toggle UX

True toggles should have one consistent representation.

Preferred rendering after charset verification:

```text
O Auto rotate
Ø Auto rotate
```

or equivalent confirmed glyphs.

If raw checkbox glyphs are not reliable, use:

```text
Auto rotate: OFF
Auto rotate: ON
```

Avoid mixing:

```text
+
-
ON
OFF
*
```

for the same semantic purpose.

SELECT should toggle exactly one setting and should not trigger unrelated side effects.

---

# 5. Enum UX

Enum entries should display their current named value.

Example:

```text
Engine: 2.0 I4
ACC resume: RES
Pedal mode: Auto
```

SELECT may cycle through a small enum.

For enums with many entries, consider entering a selector rather than requiring many repeated SELECT presses.

Avoid representing enums as pseudo-toggles.

---

# 6. Numeric Value UX

Numeric values should behave like numeric editors, not enum cycles.

Examples:

```text
Shift RPM: 3500
Launch Nm: 300
Pedal trim: +4
```

Preferred edit mode:

```text
Shift RPM
< 3500 >
```

NEXT/PREV changes the value.

SELECT accepts if necessary.

BACK exits or cancels according to existing menu conventions.

Ranges and steps should remain exactly as currently implemented unless there is a strong reason to change them.

Examples:

```text
Shift RPM:
1500–6000
step 250

Launch torque:
25–600 Nm
step 25

Pedal trim:
-10…+10
existing step
```

Do not force users to traverse a long numeric range using repeated SELECT presses.

---

# 7. Action UX

Pure actions should be visually distinct from settings.

Example:

```text
Read faults >
Reset records >
Store position >
```

SELECT executes the action.

Actions should not visually appear enabled/disabled like toggles.

---

# 8. Conditional Action UX

Actions requiring conditions should expose that condition before the user attempts them whenever the state is known.

Examples:

```text
IBS override
! Start engine
```

```text
Front brake
! Stop car
```

```text
Front brake
! Enable Dyno
```

Avoid waiting until SELECT to reveal a known precondition.

The existing availability logic should be reused where possible.

Do not duplicate vehicle-state logic in the renderer unnecessarily.

---

# 9. Pending State UX

Actions involving asynchronous vehicle behavior must distinguish:

```text
current state
requested state
pending state
failure
```

Do not present a requested state as a confirmed physical state.

A preferred pattern:

```text
4WD: ON
4WD: OFF...
4WD: OFF
```

or, if an ellipsis glyph is not available:

```text
4WD: OFF WAIT
```

Pending state should remain visible until:

```text
success
failure
timeout
cancel
```

Do not rely solely on a short 1200 ms notice for operations that remain pending longer than the notice.

---

# 10. Park Mirror — Convert from Toggle to Explicit Workflow

This is the highest priority special-case cleanup.

Currently enabling Park Mirror also stores the current mirror position.

That side effect is not obvious.

Replace this with an explicit workflow.

Preferred structure:

```text
Park mirror >
```

Submenu:

```text
Enabled: ON
Store position >
Disable >
```

or a compact equivalent.

When storing:

```text
Adjust mirror
Press SELECT
```

then:

```text
Position saved
```

The user must understand that the current mirror position is being captured.

Do not silently capture mirror position merely because a generic enable toggle changed state.

Preserve the existing low-level command:

```text
BHcmdFunctParkMirrorStoreCurPos
```

but move its invocation to an explicit capture action.

---

# 11. ACC Autostart — Rename as Mode Selection

Current modes:

```text
OFF
RES
+
```

Do not present this as a simple feature enable.

Preferred label:

```text
ACC resume: OFF
ACC resume: RES
ACC resume: +
```

Use enum semantics.

Keep existing behavior unless code review reveals a bug.

---

# 12. Remote Window Controls — Make Gesture Meaning Explicit

Current Close/Open Windows settings support:

```text
OFF
1 lock/unlock
2 locks/unlocks
```

Render them explicitly.

For example:

```text
Close: OFF
Close: 1 lock
Close: 2 locks
```

and:

```text
Open: OFF
Open: 1 unlock
Open: 2 unlocks
```

Optionally group them under:

```text
Remote windows >
```

if that improves hierarchy without adding unnecessary depth.

Do not hide gesture count behind numeric values alone.

---

# 13. Pedal Booster — Present as Mode Selection

Rename the UI concept to something like:

```text
Pedal mode: Auto
```

instead of making it look like a simple booster switch.

Current values must remain understandable, including modes such as:

```text
OFF
Auto
Bypass
A
N
D
R
Hybrid
Kids
```

If A/N/D/R require explanation, document them.

Do not change vehicle behavior during this UX refactor unless a real bug is found.

---

# 14. Pedal Trim — Numeric Editor

Present:

```text
Pedal trim: +4
```

Use numeric editing rather than SELECT cycling when practical.

Preserve existing range and step.

Do not silently wrap from maximum to minimum during normal numeric editing unless existing UX strongly depends on it.

Prefer clamping at range boundaries in edit mode.

---

# 15. Shift RPM — Numeric Editor

Convert from cyclic SELECT behavior to an explicit numeric setting.

Example:

```text
Shift RPM: 3500
```

Edit:

```text
Shift RPM
< 3500 >
```

NEXT/PREV:

```text
±250 RPM
```

Preserve the existing supported range.

---

# 16. Launch Torque — Numeric Editor

Same pattern as Shift RPM.

Example:

```text
Launch Nm: 300
```

Edit with NEXT/PREV.

Preserve current:

```text
minimum
maximum
step
```

Do not require many SELECT presses to reach a distant value.

---

# 17. Engine Profile — Keep as Enum

Current presentation is already close to correct.

Use:

```text
Engine: 2.2 D
Engine: 2.0 I4
Engine: 2.9 V6
```

This is a small enum, so SELECT cycling is acceptable.

Ensure it uses the shared enum infrastructure rather than a custom special case.

---

# 18. USB CAN / ELM327 — Convert to Exclusive Mode

Do not present mutually exclusive USB functions as independent toggles.

Replace with one enum/mode:

```text
USB mode: OFF
USB mode: CAN
USB mode: ELM327
```

Include other mutually exclusive USB personalities if the current implementation requires them.

The UI must make it impossible to imply that CAN capture and ELM327 can be independently ON at the same time.

Reuse existing underlying enable flags/migration code if changing persisted representation would be risky.

Backward compatibility with stored settings must be preserved.

---

# 19. Front Brake — Remove Hidden Side Effects

Audit the current Front Brake flow carefully.

The user should always see:

```text
current brake state
required preconditions
pending operation
interaction with Launch Assist
```

Do not silently disable Launch Assist as an unexpected consequence of pressing Front Brake.

If Launch Assist must be disabled first, show that explicitly.

Possible workflow:

```text
Front brake
! Launch active
```

then either:

```text
Disable launch first >
```

or reject the operation with a clear explanation.

Also expose known conditions such as:

```text
! Stop car
! Enable Dyno
```

before SELECT where possible.

---

# 20. 4WD — Separate Current, Requested and Pending State

Do not use a short notice as the only indication of an active 4WD sequence.

Render state explicitly:

```text
4WD: ON
4WD: OFF
4WD: OFF...
4WD: ERROR
```

If SELECT cancels an active sequence, make that semantic clear.

Do not display requested state as confirmed state until actual state logic confirms it.

Preserve existing vehicle safety conditions.

---

# 21. QV Exhaust — Do Not Claim Measured State Without Feedback

Audit exactly what the current state variable represents.

If firmware tracks only the requested sequence and does not receive physical valve feedback, do not present:

```text
Exhaust: OPEN
```

as confirmed physical state.

Prefer:

```text
Exhaust req: OPEN
```

or:

```text
Exhaust: OPEN...
```

while pending.

Only use a confirmed state label if an actual CAN signal or other feedback proves valve position.

This distinction must be preserved in code comments and tests.

---

# 22. IBS Override — Show Precondition Upfront

If RPM must be greater than the existing threshold, render that condition before the user attempts activation.

Example:

```text
IBS override
! Start engine
```

When available:

```text
IBS override: OFF
```

Reuse existing RPM and availability state.

Do not change the safety threshold as part of this UX task.

---

# 23. Fault Reader — Treat as Workflow

Group if appropriate:

```text
Faults >
```

with:

```text
Read faults >
Clear faults >
```

During operations show:

```text
Reading...
Clearing...
```

Results:

```text
No faults
3 faults
Faults cleared
```

Failures should preserve the existing detailed failure categories:

```text
ECU no reply
timeout
rejected
invalid response
protocol/send error
```

If Read is unavailable because Clear is active, show it as unavailable instead of waiting for an attempted action.

---

# 24. Audit the Entire Firmware for Similar Hidden Semantics

Do not stop after the entries listed above.

Search for every menu/setup operation where SELECT:

```text
changes more than one setting
captures runtime state
sends a vehicle command
starts an asynchronous sequence
has hidden preconditions
cycles through more than two values
changes another feature
has mutually exclusive behavior
uses a value range
uses request state as if it were confirmed state
```

Classify every discovered entry using the shared UI entry types.

If a new case does not fit any type, decide whether:

1. the UI type model is missing one genuinely useful type, or
2. the implementation is unnecessarily special and should be simplified.

Prefer option 2 when practical.

---

# 25. Separate UI Semantics from Vehicle Logic

Do not move vehicle behavior into generic UI code.

The UI layer should ask:

```text
what type is this entry?
what value should be shown?
is it available?
why is it unavailable?
what action should run?
is it pending?
```

Vehicle modules should continue deciding:

```text
whether the operation is technically valid
how CAN commands are generated
how actual state is detected
how sequences progress
```

The UX refactor must not weaken the existing architectural boundaries.

---

# 26. Avoid Hidden Cross-Feature Side Effects

Audit callbacks for patterns such as:

```c
feature_A_select()
{
    feature_B_disable();
    feature_A_enable();
}
```

If such dependency is technically required:

- expose it to the user,
- document it,
- test it.

Do not allow SELECT on one menu item to silently change another unrelated visible feature.

---

# 27. Define Interaction Rules Globally

Document and enforce:

```text
NEXT/PREV:
navigate or edit current numeric value

SELECT:
enter / toggle / execute / accept according to entry type

BACK:
exit current editor / go one level up

long BACK:
preserve existing global behavior if already defined
```

Avoid unique input behavior per feature unless unavoidable.

Do not add double-click or multiple long-press durations.

---

# 28. Use the Same Renderer for Equivalent Entry Types

Do not manually format every toggle/enum/number differently.

Create small helpers such as:

```text
render_toggle()
render_enum()
render_number()
render_action()
render_pending()
render_unavailable()
```

or equivalent.

The exact API is flexible.

The goal is to reduce semantic duplication and prevent future inconsistency.

---

# 29. Preserve Display Width Compatibility

All rendering must continue to support:

```text
18-character build
24-character LARGE_DISPLAY build
```

Do not assume MY23 implies 24 characters.

`IPC_MY23_IS_INSTALLED` and `LARGE_DISPLAY` must remain conceptually independent.

All new strings must be tested in both display widths.

---

# 30. Preserve Raw Byte Display Semantics

The dashboard protocol is byte-oriented.

Do not introduce normal UTF-8 strings into the IPC display path unless an explicit transcoding layer is intentionally added and tested.

For special symbols:

```text
store/send verified raw byte values
```

Do not write source literals that may silently become multibyte UTF-8.

Prefer definitions such as:

```c
#define IPC_GLYPH_SELECTED ((char)0xD8)
```

after hardware validation.

---

# 31. Persistence Compatibility

Do not break existing user settings unnecessarily.

If UI representation changes but stored value semantics remain valid, keep the persisted representation unchanged.

If a setting must change representation, provide migration.

Special attention:

```text
USB mode
Park Mirror
ACC mode
Pedal mode
window gesture settings
```

Existing installations should not reset configuration after firmware update without a strong reason.

---

# 32. Tests

Add or update host-side tests for each shared entry type.

At minimum:

### Toggle

```text
OFF → SELECT → ON
ON → SELECT → OFF
```

### Enum

```text
cycles valid values
does not produce invalid value
renders correct label
```

### Number

```text
NEXT increments by correct step
PREV decrements
boundaries are correct
no unintended wrap unless explicitly required
```

### Conditional action

```text
available state executes
unavailable state does not execute
reason is rendered
```

### Capture

```text
enter workflow
capture only occurs on explicit confirmation
cancel does not store state
```

### Exclusive mode

```text
selecting one mode disables conflicting mode
UI never reports two mutually exclusive modes simultaneously
```

### Pending action

```text
request state is distinguishable from confirmed state
success clears pending
failure clears pending
timeout clears pending
cancel clears pending where supported
```

---

# 33. Add Feature-Specific Regression Tests

At minimum cover:

```text
Park Mirror position capture
ACC mode cycling
remote window mode cycling
Pedal mode
Pedal trim bounds
Shift RPM editor
Launch torque editor
USB mode exclusivity
Front Brake preconditions
4WD pending/cancel
QV exhaust requested vs confirmed semantics
IBS RPM requirement
Read/Clear fault mutual exclusion
```

Use fake time and mocked/stubbed vehicle state where appropriate.

Do not require hardware for ordinary UI state-machine tests.

---

# 34. UX Consistency Audit After Migration

After implementation, enumerate every entry visible under:

```text
Favorites
Readings
Actions
Settings
Information
editors
diagnostic screens
```

For each entry record:

```text
label
UI type
SELECT behavior
NEXT/PREV behavior
availability
pending semantics
side effects
symbol
```

Any entry that cannot be explained cleanly in this table should be reviewed again.

This audit is part of the task, not optional documentation work.

---

# 35. Keep Normal Menu Simple

Do not expose implementation/debug terminology in the main user menu.

Technical details belong under:

```text
Information
Diagnostics
Debug build
hidden test menu
```

Examples:

```text
raw CAN IDs
UART states
queue counters
character byte codes
internal sequence numbers
```

should not leak into normal Actions/Settings.

---

# 36. Hidden Diagnostics / Test Menu

Add or extend a diagnostic-only menu for development features such as:

```text
IPC charset test
input test
CAN counters
UART counters
peer status
reset reason
stack information
display refresh test
```

This may be:

```text
DEBUG_MODE only
```

or hidden behind an intentional entry mechanism.

Do not clutter production UX.

---

# 37. Documentation

Document the shared UI types and symbol meanings in one architecture/UX document.

Include examples such as:

```text
Toggle:
Ø Auto rotate

Enum:
ACC resume: RES

Number:
Shift RPM: 3500

Action:
Read faults >

Unavailable:
IBS override
! Start engine

Pending:
4WD: OFF...
```

Also document confirmed raw IPC glyph byte values after hardware testing.

---

# 38. Do Not Overengineer

Avoid:

```text
dynamic allocation
generic object framework
runtime polymorphism
deep menu descriptor hierarchies
large callback abstraction layers
new UI framework
RTOS/task changes
vehicle logic relocation unrelated to this work
```

BACCAble runs on a constrained STM32F072.

Prefer small static descriptors and simple switch/helper logic.

---

# 39. Implementation Order

Recommended sequence:

## Phase A — Foundations

1. Introduce UI entry type definitions.
2. Introduce shared symbol definitions.
3. Add common rendering helpers.
4. Add charset diagnostic screen.
5. Add unit tests for generic entry behavior.

Do not change all feature behavior yet.

## Phase B — Simple Settings

Migrate:

```text
normal toggles
Engine profile
ACC resume
window modes
Pedal mode
Pedal trim
Shift RPM
Launch torque
USB mode
```

## Phase C — Workflow Features

Migrate:

```text
Park Mirror
IBS override
Fault reader
Front Brake
4WD
QV exhaust
```

These require more careful state handling.

## Phase D — Full Audit

Search the entire codebase and classify every remaining menu/setup entry.

Remove one-off presentation logic where the shared model now covers it.

## Phase E — Hardware Validation

Test:

```text
18-char display
24-char display
IPC charset
special symbols
fast navigation
all editors
all pending states
all unavailable states
```

Then finalize the production glyph set.

---

# 40. Acceptance Criteria

The task is complete when:

- every interactive menu/setup item has a clear semantic UI type,
- toggle/enum/number/action/capture behavior is no longer visually ambiguous,
- there are no hidden state-capture operations,
- mutually exclusive settings are represented as modes rather than unrelated switches,
- known preconditions are visible before SELECT where practical,
- requested state is never mislabeled as confirmed state,
- pending operations remain visible until resolution,
- Park Mirror uses an explicit capture workflow,
- numeric settings use intuitive editing,
- shared symbols are defined centrally,
- only hardware-confirmed non-ASCII IPC glyphs are used in production,
- both 18-char and 24-char builds pass tests,
- persisted settings remain backward compatible,
- feature behavior remains unchanged unless intentionally documented,
- host-side regression tests cover the new interaction model,
- the final codebase contains significantly fewer special-case UI branches.

---

# Guiding Principle

The main rule for all future BACCAble UI development should be:

> One menu entry should have one obvious interaction model.

A user should not need to know implementation details to understand whether an entry:

- toggles,
- chooses,
- edits,
- executes,
- captures,
- waits,
- or is unavailable.

The shared UI type system and symbol vocabulary should become the default foundation for all future BACCAble features.