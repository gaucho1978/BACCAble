# BACCAble Menu UX Consistency Refactor Plan

## Objective

Unify the BACCAble menu interaction model so that navigation, editing, saving, cancellation and list numbering behave consistently across the entire UI.

The current `v5-beta-6` already has a strong foundation:

- shared `UiEntryType`,
- shared rendering helpers,
- explicit Park Mirror capture workflow,
- numeric editors,
- exclusive USB mode,
- request/pending semantics,
- conditional action handling,
- hidden diagnostics,
- host-side UX regression tests.

Do **not** replace or redesign those systems.

The remaining problem is the global interaction model.

Today, the same gestures have different meanings depending on context:

- BACK sometimes navigates,
- BACK sometimes cancels,
- BACK sometimes saves,
- SELECT sometimes executes,
- SELECT sometimes navigates backward,
- Settings has `Save`,
- Feature Setup has `Save and back`,
- leaving some contexts implicitly saves,
- list numbering exists only in selected views.

The goal of this refactor is to establish one consistent mental model.

---

# Core UX Contract

After this refactor, the entire UI should follow these rules.

## NEXT / PREV

```text
Browse mode:
NEXT/PREV → move to next/previous peer item

Edit mode:
NEXT/PREV → modify the draft value
```

## SELECT

```text
Submenu     → enter
Toggle      → toggle
Enum        → choose/cycle value
Number      → enter edit / commit edit
Action      → execute
Capture     → confirm capture
Status/info → no-op unless explicitly actionable
```

SELECT must never silently act as BACK.

## BACK

Physical long-press currently maps to `MENU_BACK`.

Its semantic meaning must be:

```text
BACK → cancel current unfinished workflow
or
BACK → move exactly one hierarchy level upward
```

BACK must never itself mean “Save”.

## Persistence

```text
Commit changes in RAM
→ mark corresponding state dirty

Leave configuration context / close menu / idle timeout
→ persist dirty state automatically

Successful persistence
→ silent

Failed persistence
→ explicit error and remain recoverable
```

The user should not normally interact with Flash persistence explicitly.

---

# Phase 0 — Baseline and Inventory

Before changing behavior, inspect the current `v5-beta-6` implementation and create a behavioral inventory.

Review at minimum:

```text
firmware/baccable/features/menu.c
firmware/baccable/features/menu_input.c
firmware/baccable/features/ui_entry.c
firmware/baccable/features/ui_entry.h
firmware/baccable/settings/setup_menu.c
firmware/baccable/settings/setup_entries.c
firmware/baccable/settings/persistence.c
firmware/baccable/features/menu_diagnostics.c
tests/test_menu.c
tests/test_setup_ui.c
tests/test_unified_ui.c
```

For every `MenuView`, document:

```text
NEXT/PREV behavior
SELECT behavior
BACK behavior
whether it saves
whether it cancels
whether it has explicit Save
whether it is a list
whether it shows x/y
```

Do not start by editing strings.

First establish the exact current behavior.

---

# Phase 1 — Define the Global Interaction Contract in Code/Tests

Before changing implementation, add or update documentation/tests that define the intended behavior.

Create a simple contract table equivalent to:

| View | NEXT/PREV | SELECT | BACK |
|---|---|---|---|
| ROOT | move | enter | close |
| FAVORITES | move | no-op | ROOT |
| GROUPS | move | enter | ROOT |
| VALUES | move | no-op | GROUPS |
| ACTIONS | move | execute | ROOT |
| SETTINGS | move | enter/change | ROOT |
| SETUP | move/edit | interact | SETTINGS / cancel nested workflow |
| EDIT_FAVORITES | move | toggle | SETTINGS |
| EDIT_VISIBLE | move | toggle | SETTINGS |
| ORDER_FAVORITES | move/reorder | pick/drop | SETTINGS |
| INFORMATION | move | enter actionable item only | ROOT |
| FAULTS | move | operation-specific | ACTIONS |
| DIAGNOSTICS | move | diagnostic action | INFORMATION |

This contract should become the source of truth.

Do not preserve inconsistent old behavior merely because current tests assert it.

---

# Phase 2 — Remove Explicit Save Entries

Remove persistence as a visible menu concept.

## Remove Settings → Save

Delete the explicit `Save` entry from Settings.

The Settings list should contain only actual settings/editors.

If the current list is:

```text
Feature setup
Edit favorites
Visible pages
Order favorites
Sort...
Save
```

after refactor it should become:

```text
Feature setup
Edit favorites
Visible pages
Order favorites
Sort...
```

Update indexes and tests accordingly.

## Remove Feature Setup → Save and back

Delete:

```text
< Save and back
```

from Feature Setup.

Do not replace it with another explicit save entry.

BACK should leave Feature Setup.

Persistence should happen automatically if committed changes exist.

---

# Phase 3 — Introduce Dirty Tracking

Add explicit dirty state.

Minimum required:

```c
settings_dirty
preferences_dirty
```

or equivalent state local to the relevant subsystem.

Mark `settings_dirty` when committed settings change, including:

```text
Feature Setup toggles
enum changes
numeric commits
USB mode changes
ACC mode changes
Pedal mode
Park Mirror enabled state
other persisted setup values
```

Mark `preferences_dirty` when committed preferences change:

```text
Favorites membership
Visible pages
Favorites order
Sort order
last persisted preference values if applicable
```

Do not mark dirty merely because an editor is entered.

Do not mark dirty for draft values until they are committed.

---

# Phase 4 — Separate Commit from Persistence

Preserve the current numeric draft model.

For example:

```text
Shift RPM: 3500

SELECT
→ enter edit

NEXT
→ draft = 3750

BACK
→ cancel draft, value remains 3500

or

SELECT
→ commit 3750
→ settings_dirty = true
```

Do not write Flash during every increment/decrement.

The same principle applies to other editors and workflows.

## Capture workflows

For Park Mirror:

```text
enter capture
→ no persistent side effect yet

SELECT
→ execute explicit store operation

BACK
→ cancel
```

Do not persist merely because capture mode was entered.

---

# Phase 5 — Centralize Automatic Persistence

Create a small helper, for example:

```c
menu_persist_if_dirty()
```

or separate helpers:

```c
settings_persist_if_dirty()
preferences_persist_if_dirty()
```

The exact API is flexible.

The helper should:

```text
1. check dirty state
2. save only changed subsystem(s)
3. clear dirty flag only after successful save
4. propagate failure
5. avoid unnecessary board sync / USB apply when no settings changed
```

Call this helper when appropriate:

```text
leaving Feature Setup
leaving Favorites editor
leaving Visible Pages editor
leaving Favorites order editor
leaving Settings
closing ROOT/menu
idle timeout
```

Avoid duplicate writes if several levels are exited consecutively.

Prefer one persistence point when possible.

---

# Phase 6 — Make Successful Save Silent

Remove normal:

```text
Saved
```

notices from standard navigation.

The user should not see implementation details of Flash persistence during ordinary use.

Successful flow:

```text
change setting
BACK
→ immediately return to parent
```

No mandatory delay.

Only show persistence UI on failure.

---

# Phase 7 — Redesign Save Failure Handling

Current logic should not reinterpret SELECT as BACK or otherwise overload normal navigation.

Use an explicit failure state.

Example:

```text
! Save failed
SELECT = Retry
BACK = stay/cancel exit
```

Or, if simpler:

```text
! Save failed
SELECT = Retry
```

Requirements:

- dirty flag must remain set after failure,
- failed data must not be considered persisted,
- the user must not silently leave with data assumed saved,
- retry should be explicit,
- no recursive or hidden event rewriting.

Add regression tests for:

```text
save fails
dirty remains true
retry succeeds
dirty clears
navigation resumes
```

---

# Phase 8 — Normalize BACK Everywhere

Refactor BACK behavior to follow one rule.

## Numeric editor

```text
BACK → cancel draft
```

## Park Mirror capture

```text
BACK → cancel capture
```

## Park Mirror submenu

```text
BACK → Feature Setup
```

## Feature Setup

```text
BACK → Settings
```

Automatic persistence occurs separately if dirty.

## Editors

```text
Edit Favorites → Settings
Visible Pages → Settings
Order Favorites → Settings
```

## Main hierarchy

```text
VALUES → GROUPS
GROUPS → ROOT
ACTIONS → ROOT
SETTINGS → ROOT
INFORMATION → ROOT
ROOT → close menu
```

BACK must always unwind exactly one semantic level.

Do not combine BACK and save semantics.

---

# Phase 9 — Remove SELECT-as-BACK Behavior

Audit all views for cases where SELECT performs navigation backward without an explicit actionable entry.

## FAVORITES / VALUES

Current behavior that returns to ROOT should be removed.

Preferred:

```text
SELECT → no-op
```

Future optional behavior may be:

```text
details
peak toggle
context action
```

but not backward navigation.

## INFORMATION

Normal informational pages:

```text
SELECT → no-op
```

Only explicitly actionable entries such as:

```text
IPC diagnostics >
```

should react to SELECT.

BACK handles navigation.

---

# Phase 10 — Define One Numbering Rule

The user prefers numbered menu positions.

Make numbering a global UI rule.

## Rule

Show `current/total` on any screen where NEXT/PREV traverses a list of peer items.

Examples:

```text
ROOT
GROUPS
ACTIONS
SETTINGS
FEATURE SETUP
FAVORITES
READINGS
EDIT FAVORITES
VISIBLE PAGES
ORDER FAVORITES
INFORMATION
FAULT LIST
```

Do not show `x/y` on:

```text
notices
pending states
confirmation screens
capture instructions
save errors
single-purpose workflows
charset pages that already show raw ranges
```

---

# Phase 11 — Use Visible Count, Not Static Count

The denominator must match what the user can actually browse.

Examples:

## Actions

If 11 actions exist in code but only 8 are visible/available in the current build/state:

```text
3/8
```

not:

```text
3/11
```

## Feature Setup

If setup entries are filtered by build/engine/feature availability:

```text
current / visible_setup_count
```

not raw `setup_params_count`.

## Readings

Use the filtered page list count.

## Favorites

Use actual favorites count.

---

# Phase 12 — Centralize Number Rendering

Do not duplicate:

```c
snprintf("%u/%u ...")
```

throughout the code.

Add a small helper.

Possible shape:

```c
ui_render_list_entry(
    char *out,
    size_t out_len,
    unsigned current,
    unsigned total,
    UiEntryType type,
    const char *label,
    const char *value
);
```

Or separate:

```c
ui_render_position_prefix(...)
ui_render_position_suffix(...)
```

Keep it lightweight.

Do not build a large UI framework.

---

# Phase 13 — Choose Consistent Position Layout

Use one layout rule where practical.

Preferred default:

```text
2/5 > Readings
3/8 Dyno: OFF
4/12 Oil: 98C
```

Reason:

```text
x/y = list context
symbol = entry semantics
label/value = content
```

For data-heavy views, allow a suffix form if necessary:

```text
Oil 98C 3/12
```

The renderer may choose prefix/suffix based on available width.

Support both:

```text
18-char
24-char
```

Do not let numbering remove critical value information.

---

# Phase 14 — Number Root and Groups Consistently

Keep numbering.

Standardize formatting.

Example:

```text
1/5 > Favorites
2/5 > Readings
3/5 > Actions
4/5 > Settings
5/5 > Information
```

Groups:

```text
1/7 > Engine
2/7 > Transmission
...
```

Remove one-off formatting differences between Root and Groups.

---

# Phase 15 — Number Settings

After removing Save:

```text
1/5 > Feature setup
2/5 > Edit favorites
3/5 > Visible pages
4/5 > Order favorites
5/5 Sort: A-Z
```

The denominator must reflect actual visible items.

---

# Phase 16 — Number Actions

Format according to semantic type.

Examples:

```text
1/8 QV req: AUTO
2/8 Dyno: OFF
3/8 > Read faults
```

For conditional action:

```text
4/8 ! IBS override
```

or render its unavailable reason on the second line/current screen as the current design permits.

Do not count hidden entries.

---

# Phase 17 — Number Information

Example:

```text
1/6 FW v5-beta-6
2/6 C2 v5-beta-6
3/6 BH v5-beta-6
4/6 MY23 ON 18ch
5/6 Immobilizer ON
6/6 > Diagnostics
```

Only Diagnostics reacts to SELECT.

---

# Phase 18 — Number Feature Setup

Remove the artificial `Save and back` item.

Then number real setup entries only:

```text
1/33 Auto rotate: ON
2/33 Engine: 2.0 I4
...
```

If text does not fit:

```text
1/33 Auto rot: ON
```

or suffix the counter.

Do not truncate the value before less important label text.

---

# Phase 19 — Number Readings and Favorites

Add position context.

Examples:

```text
3/12 Oil: 98C
```

or:

```text
Oil 98C 3/12
```

For grouped readings, optionally show group context only if it fits without making the value unreadable.

Do not display stale numbers as live.

Preserve current stale/no-data behavior.

---

# Phase 20 — Number Edit Lists

## Edit Favorites

Before final charset validation:

```text
5/42 Oil temp: ON
```

After a confirmed glyph set:

```text
5/42 Ø Oil temp
6/42 O Rail psi
```

## Visible Pages

Same pattern.

## Order Favorites

Example:

```text
2/6 * Oil temp
```

where `*` means current item selected for movement.

---

# Phase 21 — Number Fault Results

If multiple faults are shown:

```text
1/4 P0123 ...
2/4 P0456 ...
```

Do not add numbering to:

```text
Reading...
Clearing...
ECU no reply
```

because those are workflow/status screens, not lists.

---

# Phase 22 — Preserve Existing Good Pending Semantics

Do not regress the beta-6 request/confirmation model.

Keep distinctions such as:

```text
4WD requested
QV requested
pending
timeout
failure
confirmed state where actually available
```

Do not infer physical success from command delivery.

This part of beta-6 is correct.

---

# Phase 23 — Preserve Existing UiEntryType

Do not redesign `UiEntryType`.

The remaining problem is navigation/persistence consistency, not entry typing.

Keep:

```text
TOGGLE
ENUM
NUMBER
ACTION
CONDITIONAL_ACTION
CAPTURE
EXCLUSIVE_MODE
SUBMENU
STATUS
```

Only extend if the full-code audit finds a genuinely missing semantic type.

---

# Phase 24 — Preserve Hold Repeat Safety

Keep existing repeat behavior.

Do not allow repeat for:

```text
vehicle-control Actions
dangerous confirmations
reorder states where repetition would be ambiguous
```

Continue to allow it for long browse/edit lists where safe.

---

# Phase 25 — Keep Auto-Close Behavior but Align Persistence

On idle timeout:

```text
1. cancel unfinished draft/editor workflow
2. persist committed dirty values
3. close menu
```

Do not commit unfinished numeric drafts.

Do not save capture operations that were never explicitly confirmed.

Do not show `Saved`.

If save fails, preserve dirty state and record/report the failure appropriately.

---

# Phase 26 — Full Code Audit for Navigation Exceptions

Search for all assignments and transitions involving:

```text
view =
MENU_SELECT
MENU_BACK
save_all
settings_save
menu_preferences_save
setup_back
notice
retry_back
```

Identify every path that violates:

```text
SELECT = local action
BACK = one level/cancel
NEXT/PREV = move/edit
```

Remove or document exceptions.

There should be very few legitimate exceptions.

---

# Phase 27 — Simplify Persistence Side Effects

Review `settings_save()`.

If no settings changed, avoid:

```text
Flash write attempt
board_sync_restart()
usb_modes_apply()
```

Use dirty tracking to ensure these side effects happen only when relevant.

If only preferences changed, do not restart board synchronization unnecessarily.

---

# Phase 28 — Update Tests Before Final Cleanup

Rewrite tests around behavioral contracts.

Do not merely update expected strings.

Add tests such as:

## Global BACK contract

```text
every nested view:
BACK → exactly one parent level
```

## SELECT contract

```text
Information status page:
SELECT → no navigation

Reading:
SELECT → no navigation

Submenu:
SELECT → enter

Action:
SELECT → execute
```

## Numeric editor

```text
NEXT changes draft
BACK cancels
SELECT commits
```

## Persistence

```text
commit → dirty
BACK → save once
success → dirty cleared
no change → no save
failure → dirty remains
retry → clears after success
```

## Numbering

```text
current and total correct
filtered Actions denominator correct
filtered Setup denominator correct
Favorites denominator correct
18-char rendering valid
24-char rendering valid
```

---

# Phase 29 — Add Test Helpers for Visible Lists

If needed, expose small pure helpers for:

```text
visible action count
visible setup count
visible readings count
position mapping
```

This makes both UI and tests simpler.

Avoid duplicating filtering logic in tests.

---

# Phase 30 — Only After Behavior Is Stable, Clean Up Code Structure

Do not split files before the behavioral refactor is complete.

After tests pass and behavior is stable, consider extracting action-specific code from `menu.c`.

Recommended split:

```text
menu.c
    navigation/controller

menu_actions.c
    ActionEntry
    action_unavailable()
    action_run()
    action_status/render
    ActionRequest
```

Do not perform a larger decomposition unless clearly necessary.

This is cleanup, not a prerequisite.

---

# Recommended Implementation Order

Use this exact order to minimize rework.

## Step 1
Inventory existing navigation/save behavior.

## Step 2
Write/update interaction contract tests.

## Step 3
Remove explicit `Save` and `Save and back`.

## Step 4
Add dirty tracking.

## Step 5
Centralize persistence-on-exit.

## Step 6
Make save success silent and failure explicit.

## Step 7
Normalize BACK behavior.

## Step 8
Remove SELECT-as-BACK behavior.

## Step 9
Introduce shared numbering helper.

## Step 10
Apply numbering to Root/Groups/Settings/Information.

## Step 11
Apply dynamic numbering to Actions/Setup/Readings/Favorites/editors.

## Step 12
Update fault numbering and other secondary lists.

## Step 13
Run full 18-char and 24-char tests.

## Step 14
Perform a full source search for remaining navigation/save exceptions.

## Step 15
Only then consider `menu_actions.c` extraction.

---

# Acceptance Criteria

The refactor is complete when all of the following are true:

- `Save` is no longer a normal Settings item.
- `Save and back` no longer exists.
- committed settings are persisted automatically.
- unchanged state does not trigger unnecessary persistence.
- successful save is silent.
- save failure is explicit and retryable.
- BACK never implicitly means Save.
- BACK cancels unfinished workflow or moves exactly one level up.
- SELECT never acts as hidden BACK.
- normal Information pages ignore SELECT.
- Readings/Favorites do not use SELECT for backward navigation.
- numeric editors follow draft/commit/cancel semantics.
- Park Mirror capture still follows explicit confirm/cancel semantics.
- all true browse lists show `current/total`.
- counters use visible/filtered totals.
- notices/workflows are not numbered.
- 18-char display preserves important values.
- 24-char display remains correct.
- current request/pending semantics remain intact.
- hold-repeat remains safe.
- persistence compatibility is preserved.
- tests assert the global interaction model, not historical inconsistencies.

---

# Final UX Rules to Document

At the end, the firmware documentation should state only these rules:

```text
NEXT/PREV
Move through lists or edit the current draft value.

SELECT
Use or confirm the current item.

BACK
Cancel the current unfinished operation or go one level back.

Saving
Committed configuration changes are saved automatically.
```

And:

```text
x/y
Shows the current position in any browsable list.
```

If an entry violates those rules, it must have a strong documented reason.

---

# Non-Goals

Do not use this refactor to:

- redesign CAN logic,
- alter vehicle safety conditions,
- change 4WD/QV command semantics,
- rewrite `UiEntryType`,
- add a new UI framework,
- introduce dynamic allocation,
- add RTOS/tasks,
- redesign persistence storage format,
- change unrelated timing constants,
- change the physical input protocol.

This task is about consistency of interaction, navigation and persistence.

---

# Guiding Principle

The user should be able to learn the entire BACCAble UI with four rules:

> NEXT/PREV moves.  
> SELECT acts.  
> BACK goes back or cancels.  
> Saving happens automatically.

Everything else should be a consequence of those rules.