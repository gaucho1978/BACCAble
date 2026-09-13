# BACCAble Idle Timeout Fix and Latin-1 UI Polish Plan

## Purpose

This document defines two related UX changes for BACCAble:

1. fix the current idle-timeout behavior that can leave the dashboard blank or visually corrupted and makes NEXT/PREV appear unresponsive until BACK is pressed;
2. replace inconsistent ASCII-only menu markers with a small, hardware-verified Latin-1 glyph vocabulary so the menu looks cleaner, more OEM-like, and remains logically consistent.

The implementation should preserve the current `UiEntryType`, display-stream architecture, request/pending semantics, safety checks, persistence behavior, and 18/24-character support.

Do not turn this into a general UI rewrite.

---

# Part A — Idle Timeout / Automatic Return Bug

## Observed User Behavior

On `v5-beta-6`, after the menu is left inactive long enough for the idle timeout:

- sometimes the dashboard area becomes blank,
- sometimes one character or part of the old BACCAble text remains,
- NEXT/PREV no longer appears to do anything,
- pressing BACK makes BACCAble return to Favorites,
- after BACK, normal navigation works again.

This does **not** look like a firmware freeze.

The CPU/menu input path is still alive because BACK reopens the UI.

The problem is the current meaning of idle timeout and the display handoff performed during automatic close.

---

## Current Behavioral Problem

The current timeout path effectively performs:

```text
inactive menu
    ↓
save committed changes
    ↓
close BACCAble menu
    ↓
set menu_visible = false
    ↓
send blank display content
```

After `menu_visible` becomes false, normal navigation events are intentionally ignored.

The hidden-menu input path accepts BACK as the gesture that opens BACCAble again.

This explains the observed state:

```text
NEXT/PREV → ignored
SELECT    → ignored
BACK      → reopen BACCAble / Favorites
```

That behavior is technically consistent with the current implementation, but it is not intuitive UX for an **idle timeout**.

The user expects automatic inactivity handling to return to a safe/default BACCAble screen, not to silently disable the BACCAble UI.

---

# Required New Semantics

Distinguish two operations:

## Explicit close

An explicit user command to leave BACCAble.

Example:

```text
ROOT + BACK
```

This may:

```text
close BACCAble overlay
release dashboard area
return ownership to factory radio/RDS
```

## Idle timeout

Automatic inactivity handling.

This must **not** behave like explicit close.

The new rule must be:

```text
idle timeout
    ↓
cancel unfinished local workflow
    ↓
persist committed dirty state
    ↓
return to FAVORITES
    ↓
keep BACCAble menu/display visible
```

After timeout:

```text
NEXT/PREV
```

must immediately navigate Favorites.

No extra BACK gesture must be required.

---

# Required Timeout Behavior by Context

For ordinary browse contexts:

```text
ROOT
ACTIONS
SETTINGS
INFORMATION
GROUPS
```

timeout should return to:

```text
FAVORITES
```

For configuration/edit contexts:

```text
Feature Setup
Edit Favorites
Visible Pages
Order Favorites
```

timeout should:

```text
1. cancel unfinished draft/workflow if applicable
2. keep already committed values
3. persist committed changes
4. return to FAVORITES
```

For numeric editing:

```text
draft value
```

must be discarded if it was not confirmed by SELECT.

Example:

```text
Shift RPM = 3500
enter editor
draft = 3750
timeout
```

Expected:

```text
Shift RPM remains 3500
view = FAVORITES
menu visible = true
```

For Park Mirror capture:

```text
capture instructions shown
timeout
```

must cancel capture.

It must not store the mirror position automatically.

For an active critical/pending operation that should not be interrupted, preserve the current operation-specific behavior if required.

Do not blindly timeout out of an active vehicle-control workflow if the current code intentionally protects it.

---

# Explicit Close Must Remain Separate

Explicit closing should remain available through the normal root BACK behavior.

Expected semantic model:

```text
BACK inside hierarchy
→ one level back / cancel

BACK on ROOT
→ explicitly close BACCAble
```

Only this explicit close is allowed to:

```text
menu_visible = false
```

Idle timeout must not do so.

---

# Display Handoff Problem

The current close path uses blank text to clear BACCAble content.

That means the display is cleared as a normal BACCAble screen composed of spaces.

This is potentially fragile because the dashboard text is transmitted in 3-character fragments and competes with factory display/RDS activity.

A partially completed blank update can visually appear as:

```text
blank screen
one stale character
partial old content
```

Do not try to solve the idle bug by repeatedly transmitting more spaces.

The primary fix is:

> Idle timeout must not close or blank the display at all.

This removes the problematic handoff from the automatic timeout path.

---

# Explicit Display Release Investigation

The explicit ROOT/BACK close path may still need a better display-release mechanism.

There is code/documentation indicating an IPC clear/display information code such as `0x11`.

Do **not** change the production close protocol blindly.

Instead:

1. preserve the existing manual close behavior for now unless a reliable protocol-level release is already proven;
2. add a diagnostic experiment if useful;
3. verify on physical hardware whether an explicit clear/release command cleanly returns the area to factory radio/RDS;
4. only replace the blank-screen method after physical validation.

This investigation is separate from the idle-timeout fix.

The timeout fix must not depend on it.

---

# Timeout Acceptance Tests

Add regression tests for at least the following.

## Settings timeout

```text
open Settings
wait past timeout
```

Expected:

```text
view == FAVORITES
menu visible == true
NEXT moves to next favorite
```

## Setup timeout

```text
open Feature Setup
change a committed toggle
wait past editor timeout
```

Expected:

```text
change persisted
view == FAVORITES
menu visible == true
```

## Numeric draft timeout

```text
enter numeric editor
change draft without SELECT
wait past timeout
```

Expected:

```text
draft cancelled
original committed value preserved
view == FAVORITES
```

## Park Mirror capture timeout

```text
enter Store position workflow
do not confirm
wait past timeout
```

Expected:

```text
capture cancelled
no store command sent
view == FAVORITES
```

## Post-timeout input

Immediately after timeout:

```text
MENU_NEXT
MENU_PREVIOUS
```

must work normally.

Do not require BACK to restore input ownership.

## Display

Timeout must not call the normal display-clear path.

The last/fresh Favorites page should be rendered instead.

---

# Part B — Hardware-Verified IPC Character Set

## Hardware Findings

The physical IPC has now been tested directly using raw single-byte diagnostic output.

The observed character set is consistent with:

```text
ASCII / ISO-8859-1 (Latin-1)
```

for printable ranges.

Verified behavior:

```text
0x20–0x7E  printable ASCII
0x80–0x9F  unsupported/control range — do not use
0xA0–0xFF  printable Latin-1 glyphs
```

The `0x80–0x9F` CP1252-specific printable characters must **not** be used.

For example, do not use:

```text
0x85 as …
0x95 as •
0x8B as ‹
0x9B as ›
```

The physical IPC does not render those as the Windows-1252 characters.

---

# Verified Useful Glyphs

The following values have been visually verified on the physical IPC.

```c
#define IPC_GLYPH_PREV       ((char)0xAB) /* « */
#define IPC_GLYPH_NEXT       ((char)0xBB) /* » */
#define IPC_GLYPH_DEGREE     ((char)0xB0) /* ° */
#define IPC_GLYPH_PLUSMINUS  ((char)0xB1) /* ± */
#define IPC_GLYPH_DOT        ((char)0xB7) /* · */
#define IPC_GLYPH_CROSS      ((char)0xD7) /* × */
#define IPC_GLYPH_CHECKED    ((char)0xD8) /* Ø */
#define IPC_GLYPH_UNCHECKED  ((char)0x4F) /* O */
```

Do not use UTF-8 source literals such as:

```c
"Ø"
"°"
"«"
```

unless an explicit transcoding layer exists.

The dashboard transport is byte-oriented.

Use raw single-byte constants.

For example:

```c
char text[] = {
    IPC_GLYPH_CHECKED,
    ' ',
    'A','u','t','o',
    '\0'
};
```

---

# Centralize the Glyph Vocabulary

Create or extend one shared UI glyph header.

Prefer a single source of truth, for example:

```text
features/ui_glyphs.h
```

or the existing `ui_entry.h` if that remains clean.

Do not scatter magic values such as:

```c
0xD8
0xB0
0xAB
```

throughout the codebase.

Suggested definitions:

```c
#define UI_GLYPH_UNCHECKED ((char)0x4F) /* O */
#define UI_GLYPH_CHECKED   ((char)0xD8) /* Ø */

#define UI_GLYPH_PREV      ((char)0xAB) /* « */
#define UI_GLYPH_NEXT      ((char)0xBB) /* » */

#define UI_GLYPH_DEGREE    ((char)0xB0) /* ° */
#define UI_GLYPH_DOT       ((char)0xB7) /* · */

#define UI_GLYPH_CROSS     ((char)0xD7) /* × */
#define UI_GLYPH_PLUSMINUS ((char)0xB1) /* ± */
```

---

# Visual Language

Use a small, consistent set.

Do **not** use every available Latin-1 character simply because the IPC supports it.

The goal is predictable visual semantics.

---

# 1. Boolean / Checkbox State

Replace textual or inconsistent boolean markers where the checkbox representation is clearer.

Use:

```text
O = disabled / unchecked
Ø = enabled / checked
```

Examples:

```text
O Auto rotate
Ø Auto rotate

O Advanced pages
Ø Advanced pages
```

Preferred implementation:

```c
state ? UI_GLYPH_CHECKED : UI_GLYPH_UNCHECKED
```

This should primarily be used in:

```text
Feature Setup toggles
Edit Favorites
Visible Pages
other true binary selections
```

Do not use `O/Ø` for:

```text
enum values
vehicle actions
pending commands
numeric values
status-only information
```

A toggle must actually be a boolean.

---

# 2. Editors: Selected / Editing State

Do not reuse `Ø` to mean “currently editing”.

Keep editing state semantically distinct from checked state.

The current ASCII:

```text
*
```

is acceptable for:

```text
selected for move
currently editing
```

Example:

```text
2/6 * Oil temp
```

If later a better verified glyph is desired, test it separately.

For now:

```text
* = temporary editor/selection state
Ø = persistent true/on/checked state
```

This distinction is important.

---

# 3. Previous / Next

Use:

```text
«
»
```

only when they describe value navigation or directional choices inside an editor.

Example:

```text
« 3500 »
```

or a compact equivalent.

Good candidates:

```text
Shift RPM editor
Launch torque editor
Pedal trim editor
small enum selector if explicitly rendered as left/right choice
```

Do not add `« »` to ordinary list entries where NEXT/PREV navigation is already implicit.

Avoid visual clutter.

---

# 4. Temperature

Use the verified degree glyph.

Preferred:

```text
98°C
91°C
```

instead of:

```text
98C
91C
```

Use:

```c
UI_GLYPH_DEGREE
```

for temperature units.

Audit parameter formatting so that Celsius values can render:

```text
<number><degree>C
```

without UTF-8.

Do not change scaling or parameter semantics.

Only formatting changes.

---

# 5. Separator

Use:

```text
·
```

as a compact neutral separator when multiple status values share one screen.

Good examples:

```text
C2 OK · BH OK
V6 · AUTO
Oil 98° · 4.1bar
```

Do not replace normal punctuation everywhere.

Use it where it materially improves readability.

Do not use it as a bullet for every menu item.

---

# 6. Error / Failure Symbol

`×` is verified and may be used for hard failure where it improves clarity.

Suggested meaning:

```text
× = failed / invalid / unavailable result
! = warning / attention / precondition
? = unknown / no reply
```

Examples:

```text
! Start engine
? BH no reply
× Read failed
```

Do not replace all existing `!` with `×`.

Keep semantic distinction:

```text
! = user attention / unmet condition
× = actual failure
? = unknown/unavailable state
```

---

# 7. Plus/Minus

`±` is verified.

Use sparingly.

Potential use:

```text
Pedal trim ±
correction/tolerance display
```

Do not use it simply to decorate ordinary numeric editors.

The value itself should remain explicit:

```text
Pedal trim: +4
```

rather than:

```text
Pedal trim ±4
```

unless the latter is genuinely clearer.

---

# 8. Submenu / Action Indicator

Keep ASCII:

```text
>
```

for:

```text
submenu
enter
explicit action
```

Examples:

```text
3/5 > Actions
6/6 > Diagnostics
Store position >
Read faults >
```

Do not replace this with `»`.

Reason:

```text
> = semantic "enter/do this"
» = directional value navigation
```

Keep those meanings distinct.

---

# 9. Back

BACK is a physical gesture and generally should not need a persistent on-screen symbol.

Do not add `<` entries such as:

```text
< Save and back
```

The UX consistency refactor already moves away from explicit back/save entries.

If a rare explicit Back row remains necessary in a nested diagnostic screen, use plain:

```text
< Back
```

but prefer physical BACK navigation.

Do not use `«` for hierarchy back.

Reserve `«` for previous/decrement navigation.

---

# 10. Pending State

Do not use CP1252 ellipsis (`0x85`).

It is not supported by this IPC.

Use safe ASCII:

```text
WAIT
```

or three literal ASCII periods:

```text
...
```

Examples:

```text
4WD req: OFF WAIT
QV req: OPEN WAIT
```

Keep the current request-vs-confirmed semantics.

Do not imply physical success.

---

# Recommended Final Symbol Semantics

Use this as the canonical UI vocabulary:

```text
O   unchecked / disabled boolean
Ø   checked / enabled boolean

>   submenu / explicit action
*   currently selected/editing/moving

!   warning / precondition / attention
?   unknown / offline / no reply
×   hard failure

«   previous/decrement inside editor
»   next/increment inside editor

°   degrees
·   compact separator
```

Do not assign multiple meanings to the same glyph.

---

# Apply the Glyphs to Existing UI

Audit all current menu renderers.

## Feature Setup toggles

Convert true boolean entries from mixed:

```text
ON/OFF
+/-
```

where appropriate to:

```text
Ø Label
O Label
```

Do not convert enums.

Examples:

```text
Ø Auto rotate
O Advanced pages
```

## Edit Favorites

Use:

```text
5/42 Ø Oil temp
6/42 O Rail psi
```

## Visible Pages

Same convention:

```text
8/64 Ø Oil pressure
9/64 O Raw sensor
```

## Order Favorites

Keep temporary selection separate:

```text
2/6 * Oil temp
```

Do not use Ø for the movable item.

## Numeric editors

Use `«/»` only in editor mode if the layout still fits:

```text
Shift RPM
« 3500 »
```

For 18-character screens, prioritize the numeric value over decoration.

If it does not fit cleanly, retain the current editor format.

## Temperature values

Use:

```text
Oil 98°C
Coolant 91°C
```

where space permits.

## System health

Potential compact form:

```text
C2 OK · BH OK
```

if/when a combined health screen exists.

## Errors

Use:

```text
! Start engine
? BH no reply
× Read failed
```

where semantically accurate.

---

# Do Not Change These Semantics

The glyph migration is visual only.

Do not change:

```text
vehicle safety conditions
4WD behavior
QV command behavior
Fault Reader protocol
Park Mirror command behavior
settings persistence format
CAN decoding
USB mode behavior
```

unless a real bug is discovered independently.

---

# 18-Character and 24-Character Constraints

Every changed renderer must be tested in:

```text
18-char
24-char LARGE_DISPLAY
```

Rules:

1. preserve important value/state before decorative glyphs;
2. never truncate a critical numeric value just to keep `« »`;
3. `x/y` numbering remains higher priority than optional decorative separators where the consistency plan requires numbering;
4. no multibyte UTF-8 must enter the IPC string;
5. all strings must remain correctly space-padded to prevent stale trailing characters.

---

# Charset Regression Tests

Add host tests that verify the actual byte values.

Examples:

```c
assert((uint8_t)UI_GLYPH_CHECKED == 0xD8);
assert((uint8_t)UI_GLYPH_DEGREE == 0xB0);
assert((uint8_t)UI_GLYPH_PREV == 0xAB);
assert((uint8_t)UI_GLYPH_NEXT == 0xBB);
```

Verify rendered output by byte, not by UTF-8 source string.

For example, a checked toggle should contain exactly one byte:

```text
D8
```

not:

```text
C3 98
```

---

# Keep the Charset Diagnostic Screen

Do not remove the raw-byte diagnostic feature.

It is useful for validating other IPC variants.

Document the hardware observation:

```text
tested IPC:
0x20–0x7E printable ASCII
0x80–0x9F unsupported/control
0xA0–0xFF Latin-1-like printable glyphs
```

Do not assume every future IPC revision is identical without testing.

---

# Recommended Implementation Order

## Phase 1 — Fix timeout behavior first

1. add regression test reproducing the current timeout bug;
2. change idle timeout from `close menu` to `return to FAVORITES`;
3. keep `menu_visible = true`;
4. cancel unfinished editor/capture workflows;
5. persist committed values;
6. verify NEXT/PREV works immediately after timeout;
7. ensure timeout path never clears/blanks the display.

This should be a standalone logical fix.

## Phase 2 — Introduce central glyph definitions

1. add shared raw-byte glyph constants;
2. document verified Latin-1 mapping;
3. add byte-level tests.

Do not change all renderers yet.

## Phase 3 — Migrate boolean UI

Apply `O/Ø` to:

```text
Feature Setup true toggles
Edit Favorites
Visible Pages
```

Audit that every converted entry is truly boolean.

## Phase 4 — Migrate value/status formatting

Add:

```text
°
·
×
```

where semantically useful.

Do not overuse symbols.

## Phase 5 — Numeric editor polish

Evaluate `« value »` on both 18- and 24-char builds.

Keep it only if readability improves.

## Phase 6 — Full rendering audit

Search for:

```text
"+ "
"- "
"ON"
"OFF"
"* "
"! "
"? "
```

and classify each occurrence.

Do not mechanically replace text.

Verify that each symbol matches the semantic vocabulary defined above.

---

# Final Acceptance Criteria

The work is complete when:

## Idle timeout

- inactivity never leaves BACCAble in a visually blank/half-cleared state;
- idle timeout returns to Favorites;
- menu remains visible;
- NEXT/PREV works immediately after timeout;
- unfinished numeric edits are cancelled;
- unfinished Park Mirror capture is cancelled;
- committed changes are persisted;
- no BACK press is required to recover;
- explicit ROOT/BACK close remains separate.

## Latin-1 UI

- raw verified glyph constants are centralized;
- no production UI relies on CP1252 `0x80–0x9F`;
- no UTF-8 multibyte symbol accidentally enters dashboard strings;
- boolean toggles use one consistent `O/Ø` vocabulary;
- `*` remains reserved for temporary edit/move selection;
- `>` remains submenu/action;
- `!`, `?`, and `×` have distinct meanings;
- temperature uses `°` where practical;
- `·` is used only as a compact separator;
- `«/»` are used only for directional editor/value navigation;
- all affected screens are correct in both 18-char and 24-char builds.

---

# Guiding Principles

The timeout rule should be:

> Inactivity returns the user to a safe BACCAble home view.  
> Only an explicit user action closes BACCAble.

The symbol rule should be:

> One glyph, one meaning.

The display rule should be:

> Use raw single-byte characters verified on the physical IPC, never assumed UTF-8 or CP1252 behavior.
