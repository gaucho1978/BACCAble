# BACCAble Menu UX Improvement Plan

## Review and incremental delivery (2026-09-12)

Reviewed against `b05b3f6` (after v5-beta-3). The numbered proposals below are
background, not a requirement to implement everything. Use this assessment first.

| Items | Assessment and order |
| --- | --- |
| 20: regression tests | First. Extend the existing host harness; no new framework. Existing tests already cover latest-target streaming, fairness, retries, padding, stale readings and engine/favorite compatibility. |
| 5: BACK threshold | First increment: 1200 ms instead of 800 ms, with exact-boundary and tick-wrap tests. This is a deliberate usability tradeoff, not a fix for lost CAN reports. |
| 8: unchanged saves | Flash wear already addressed by `record_save()` payload comparison. First increment adds erase/program assertions, including interrupted-save recovery. Settings saves still synchronize boards and apply USB mode; skipping that is a separate behavior change. |
| 4: input timeout | Measure before changing. 300 ms detects missing reports, not maximum press length. Increasing it can turn a late release into an unintended SELECT. First increment tests 300/301 ms boundaries. |
| 19: rotation | Already resets on manual page selection through `select_page()`. First increment tests rapid navigation and the exact 5-second boundary. |
| 3, 16: wording/symbols | Next small task: distinguish requested versus confirmed state. Never replace `Req ON` with `ON` without confirmation. Keep ASCII, both screen widths, and `<` for back. |
| 2, 17, 18: feedback | Immediate render and dismissible notices exist. Pending/final results need action-specific evidence; do one action at a time. Cosmetic notices must not imply vehicle confirmation. |
| 12: diagnostic errors | Useful independent task after tests. Start with fault-reader timeout/rejection messages; preserve the state machine and retry limits. |
| 9: navigation context | Action, setting and information indices already live in RAM and are not reset on ordinary re-entry. Add observable re-entry tests before proposing new state. |
| 11, 14: menu roles | Largely satisfied by Actions/Settings/Information and Advanced pages. Preserve the current structure. |
| 13: missing readings | Cache expiration and `--` rendering already tested. Further work should address a demonstrated source-specific freshness issue. |
| 1: faster fragments | Hardware experiment later. Keep 50 ms for now. Dirty fragments reduce obsolete work but cannot make a fragmented dashboard update atomic. No host test can certify 20–30 ms dashboard compatibility. |
| 6: hold repeat | Implemented with a navigation-only allowlist, fresh-report guard and no catch-up bursts. Vehicle feel still needs validation. |
| 7: auto-close | Implemented: 30 s navigation, 60 s editors, persistent readings, diagnostic/save-error guards and retryable display clear. |
| 10: pinned actions | Defer: new preference semantics and extra UX complexity without demonstrated need. |
| 15: system health | Defer until optional board presence and heartbeat/version compatibility rules are established. Missing optional hardware must not become a false warning. |

### Small agent tasks, in order

Run only one implementation task at a time. An independent read-only review or
small test addition may run alongside it. Give agents this section and the named
files; do not pass the whole conversation or ask for a fresh repository audit.

| Task | Scope and acceptance | Stop boundary |
| --- | --- | --- |
| A: regression baseline + BACK | `menu_input.c`, `test_menu.c`, `test_core.c`, menu guide. Exact 1199/1200 ms hold boundary, no SELECT after BACK, stream loss, tick wrap, rapid wheel input with busy UART, rotation and unchanged save tests. | Completed in this branch. No other timing changes. |
| B: status wording (first pass complete) | `menu.c` action status/rendering and focused `test_menu.c` assertions. Inventory requested/confirmed/unknown states first; improve only labels whose meaning is supported. Both display widths. | Implemented explicit 4WD/QV request labels and clear-request WAIT; no new action state machine or command changes. |
| C: diagnostic error feedback (complete) | `fault_reader.c/.h`, its menu renderer and existing fault-reader tests. Distinguish no reply/rejection only where available; test timeout and recovery. | Implemented timeout, ECU rejection, invalid reply and unqueued-send messages with retry recovery tests. No transport refactor or new logging infrastructure. |
| D: navigation re-entry tests (complete) | `test_menu.c`; test returning to Actions, Settings and Information. Change `menu.c` only for a reproducible context-loss defect. | Existing behavior passed: section re-entry, close/reopen, Information SELECT return and unavailable remembered action. Test-only change; no new persistent fields or Favorites format. |
| E: input/display measurements | Capture input-report gaps and dashboard behavior at existing pacing; compare a separate 30 ms experimental build only after baseline. | No release timing change without vehicle results. |

Each task starts with a failing regression for the intended change, or a passing
characterization test proving it already works. Reuse existing tests. Each agent
returns the diff summary, commands/results and remaining uncertainty, then stops;
it must not start the next task. If a task needs a new persistence format, protocol
assumption or broad refactor, report that dependency instead of expanding scope.
These boundaries limit work; actual token consumption depends on the agent and
cannot be guaranteed by this document.

### Next beta scope (2026-09-13)

A–D and the Actions test-report improvements are merged. This branch completes
the next bounded package: navigation-only hold repeat (6), inactivity closure
with save/error/diagnostic guards (7), and differentiated, dismissible notices
(17–18). It also retries a rejected clear after closing the menu. Host tests cover
both widths, exact timers, wraparound, exclusions and retry paths.

| Remaining proposal | Release decision |
| --- | --- |
| 1: 20–30 ms display pacing | Keep 50 ms until a separate vehicle comparison confirms reliability. No claim of faster physical refresh. |
| 4: longer input-stream timeout | Keep 300 ms; need measured report gaps before relaxing lost-release protection. |
| 2: confirmed results for every vehicle action | Existing immediate acknowledgement retained. Further completion messages need actual ECU/peer confirmation, not inferred success. |
| 10: pinned actions | Not required for this package. Existing Favorites and remembered Actions remain; avoid a new persistence format. |
| 15: aggregate system health | Existing Information peer pages retained. A summary needs rules for optional boards, fresh replies and version compatibility before it can safely claim SYSTEM OK. |

This is a complete release package, not a claim that every optional proposal below
has been implemented. Vehicle smoke tests must cover hold repeat, editor timeout,
RES/BACK and factory-display handover before treating the UX as hardware-validated.
Do not expand this branch into transport, persistence-format or board-health work.

### First-increment release gate

The first increment is independently releasable; tasks B–E are not prerequisites.
Run `make -C tests -j2 test`, compile C1/C2/BH/CAN, and run firmware lint. The existing
release workflow must then pass on the merged commit. Use an immutable new beta
tag and notes stating that BACK now needs 1.2 seconds. Verify short RES, held RES
and navigation in the vehicle. Do not claim this increment fixes the previously
reported freeze or guarantees atomic screen refresh. Do not publish a release
from an agent's intermediate working tree.

---

## Purpose

This document is a focused implementation brief for improving the BACCAble menu UX.

It is intended for a coding agent or developer who should **not spend time rediscovering the entire menu architecture from scratch**. The points below are based on a recent review of the current `master` branch and the existing host-side tests.

The goal is to make the menu feel:

- immediate,
- predictable,
- consistent,
- easy to understand at a glance,
- resistant to lost input,
- resistant to stale or mixed display content,
- comfortable to use with minimal driver attention.

The implementation details below are **guidance, not a rigid design**. If the current architecture suggests a simpler or safer solution, prefer that and document the reasoning.

---

## Primary Code Areas

Focus mainly on:

- `firmware/baccable/features/menu.c`
- `firmware/baccable/features/menu_input.c`
- `firmware/baccable/features/display_stream.c`
- `firmware/baccable/features/display_stream.h`
- `firmware/baccable/features/body.c`
- `firmware/baccable/features/dashboard.c`
- `firmware/baccable/diagnostics/parameter_cache.c`
- `firmware/baccable/diagnostics/fault_reader.c`
- `firmware/baccable/settings/persistence.c`
- `firmware/baccable/storage/flash_records.c`
- existing tests under `tests/`

Do not perform a broad refactor unless it is clearly necessary for one of the items below.

---

## Known Current State

Some earlier UX work is already present and should be preserved:

- root menu uses `Actions` rather than the older `Functions` naming,
- engine profile support already distinguishes I4 / V6 / diesel behavior,
- `Advanced pages` already exists,
- `Maximum hold` / peak functionality is already exposed,
- action availability already provides several user-facing notices,
- reading/favorite page position is already remembered in some cases,
- display streaming already uses a latest-target / dirty-fragment style mechanism,
- failed display transmission is retried rather than blindly accepted,
- menu events generally trigger immediate re-rendering.

Do not re-implement these features from scratch.

---

# 1. Reduce Display Update Latency

The display path still sends text in 3-character fragments with pacing in `body.c`.

Evaluate reducing the current fragment interval from roughly 50 ms to a safer lower value, for example 20–30 ms, if the dashboard and CAN traffic tolerate it.

Goals:

- typical menu navigation should visibly react in under ~150–250 ms,
- avoid excessive CAN load,
- preserve retry behavior,
- preserve fragment fairness,
- do not introduce flicker or unstable display behavior.

If 50 ms is required for reliability, keep it and document why.

---

# 2. Make Input Feedback Immediate

Every accepted user action should produce immediate visible feedback, even if the underlying vehicle operation takes longer.

Preferred interaction model:

```text
Action selected
→ immediate acknowledgement
→ pending state if required
→ confirmed final state
```

Example:

```text
Dyno
→ Confirm RES
→ Dyno...
→ Dyno ON
```

Avoid situations where the user presses a button and sees no obvious response.

---

# 3. Standardize Status Language

Audit menu labels and status strings.

Avoid mixing developer-oriented and user-oriented terms such as:

```text
Req ON
Req OFF
RES
```

Prefer a small, consistent vocabulary such as:

```text
ON
OFF
OPEN
CLOSED
WAIT
READY
ERROR
```

Use `RES` only when it is an instruction to press the RES button, not as a functional state.

Keep strings short enough for the dashboard.

---

# 4. Revisit the 300 ms Input Timeout

`menu_input_update()` currently has logic that can discard an active gesture after a relatively short gap in input reports.

This can potentially turn a valid slower press/release into a lost click.

Review the semantics and separate:

- loss-of-input-stream detection,
- normal click timing,
- long-press detection.

A longer stream timeout, e.g. roughly 700–1000 ms, may be more tolerant, but choose values based on the actual input protocol.

Add regression tests for timing boundaries.

---

# 5. Increase the Long-Press BACK Threshold

The current long-press threshold is close enough to a slow normal press that the boundary may feel unpredictable.

Consider moving BACK long-press detection from roughly 800 ms toward ~1200–1500 ms.

Requirements:

- short press remains SELECT,
- long press remains BACK,
- one physical gesture must never produce both,
- timing boundary must be covered by tests.

---

# 6. Add Hold-to-Repeat Navigation

Long lists currently require repeated individual wheel actions.

Implement auto-repeat where appropriate:

```text
initial movement → immediate NEXT/PREV
hold ~500 ms
→ repeat every ~150–200 ms
```

Use it for list navigation such as:

- Settings,
- Visible pages,
- Favorites editing,
- Readings,
- other long lists.

Do not enable repeat for actions where repetition could be unsafe.

---

# 7. Add Menu Auto-Close on Inactivity

Use the existing input activity timestamp to automatically close the menu after inactivity.

Suggested behavior:

- normal menu: ~20–30 s,
- editor / diagnostic screens: longer timeout if needed,
- do not close while a confirmed operation is actively running,
- closing the menu should cleanly release the display back to factory radio/RDS behavior.

Avoid duplicating existing inactivity state if one already exists elsewhere.

---

# 8. Avoid Saving Flash When Nothing Changed

The current menu flow can trigger settings/preferences persistence even when no user-visible value changed.

Introduce change tracking, for example:

```text
settings_dirty
preferences_dirty
```

or an equivalent mechanism.

Benefits:

- less blocking work,
- less Flash wear,
- fewer synchronization side effects,
- better menu responsiveness.

Do not write to Flash just because the user exits a menu level.

---

# 9. Preserve More Navigation Context

Readings/Favorites already preserve some position state.

Extend the same idea where useful to:

- last Action,
- last Setting,
- last Information page.

Prefer RAM-only restoration unless persistence across power cycles is clearly useful.

The user should generally return close to where they left off instead of always starting from the first entry.

---

# 10. Minimize Steps for Frequent Tasks

Review the number of interactions required for common runtime tasks.

Frequently used operations should generally be reachable in one or two navigation steps.

Possible approaches:

- preserve the existing Favorites concept,
- allow favorite/pinned actions,
- reorder common runtime sections before rarely used setup sections.

Do not add unnecessary menu depth.

---

# 11. Keep Runtime Actions Separate from Setup

Preserve a clear mental model:

```text
Drive/runtime:
- Favorites
- Readings
- Actions

Configuration:
- Settings
- Information
```

Avoid mixing one-time setup options with operations a user may invoke while driving.

Naming, ordering, and navigation should reinforce this distinction.

---

# 12. Improve Diagnostic Error Messages

`fault_reader` should expose the reason for failure instead of collapsing different failures into a generic message.

Where technically possible, distinguish cases such as:

```text
ECU no reply
Timeout
Rejected
Invalid response
Protocol error
```

Keep the internal state machine simple if possible.

Do not expose low-level UDS details unless they help the user.

---

# 13. Make Stale / Missing Parameter Data Obvious

The parameter cache already knows when data is stale.

Ensure the UI never leaves an old numeric value looking current after the source has stopped updating.

Use a clear representation such as:

```text
--
NO DATA
```

or another short equivalent.

If useful, distinguish:

- valid/live,
- stale,
- never received.

Do not add visual complexity unless it clearly improves trust in the reading.

---

# 14. Keep Technical Details Inside Information/Diagnostics

Runtime menu entries should use user-oriented wording.

Detailed technical information such as:

- firmware versions,
- peer version numbers,
- protocol/display metadata,
- raw diagnostic state,

belongs in `Information` or a developer/advanced section.

The normal menu should communicate actions and results, not implementation details.

---

# 15. Add a Clear Overall System Health State

The code already knows peer versions and peer availability.

Add a concise high-level status such as:

```text
SYSTEM OK
BH OFFLINE
C2 OFFLINE
VERSION MISMATCH
```

Avoid requiring the user to inspect several separate information pages just to know whether the installation is healthy.

Detailed versions can remain on secondary pages.

---

# 16. Define and Enforce a Small Symbol Vocabulary

The menu already uses symbols such as:

```text
>
+
-
*
!
?
```

Formalize their meaning and use them consistently.

Suggested interpretation:

```text
>  enter / submenu
+  enabled / positive state
-  disabled / negative state
*  selected / editing
!  warning / attention
?  unknown / unavailable
```

If the dashboard character set supports better single-byte glyphs, they may be used, but do not depend on UTF-8.

---

# 17. Use Different Feedback Durations for Different Message Types

Avoid one fixed duration for every notice.

Suggested categories:

- simple toggle confirmation: ~600–800 ms,
- warning/error: ~1500–2000 ms,
- pending operation: visible until success/failure or explicit timeout.

Normal navigation should remain responsive and should be able to dismiss non-critical notices.

---

# 18. Do Not Block Navigation for Cosmetic Feedback

A successful toggle notice should not make the menu feel locked.

Preserve the existing behavior where normal user input can interrupt non-critical notices.

Only safety-critical confirmations or active operations may temporarily block navigation.

Be careful not to introduce modal states unnecessarily.

---

# 19. Make Auto-Rotation Respect Recent User Interaction

If automatic reading rotation is enabled, recent user input should postpone the next automatic page change.

Reset or extend the auto-rotation timer after relevant navigation input so the page does not change just after the user intentionally selected it.

The UI should never feel like it is “fighting” the user.

---

# 20. Add UX Regression Tests

Add host-side tests for observable interaction behavior, not only internal logic.

Important scenarios include:

- rapid NEXT/NEXT/NEXT input does not lose events,
- short press and long press produce exactly one expected action,
- timeout boundaries are deterministic,
- new display target supersedes stale content,
- failed transport send is retried,
- shortened text does not leave stale trailing characters,
- stale parameter data stops displaying an old value,
- menu auto-close respects active operations,
- save without changes does not write Flash,
- auto-rotation is postponed by manual interaction.

Where timing is involved, use fake time instead of wall-clock delays.

---

## UX Performance Targets

Use these as practical targets rather than hard protocol requirements:

- visible response to accepted input: preferably <150 ms, always clearly acknowledged within ~250 ms,
- no lost normal button presses,
- no mixed old/new display content,
- no stale numeric value presented as live,
- no unnecessary Flash write on menu exit,
- no ambiguous status strings,
- no accidental SELECT/BACK overlap,
- no menu state that requires a power cycle to recover.

---

## Safety and Compatibility Constraints

Do not weaken existing safety checks.

Preserve:

- confirmation for potentially risky actions,
- vehicle-state guards,
- master-enable / availability checks,
- CAN/UART retry behavior,
- stable parameter/page IDs,
- existing Favorites compatibility,
- engine-profile filtering,
- Advanced pages behavior.

Do not introduce automatic execution of vehicle-control actions.

---

## Implementation Guidance

Prefer:

- small, isolated patches,
- existing abstractions,
- host-testable logic,
- fake time in tests,
- explicit state rather than hidden timing side effects.

Avoid:

- large architecture rewrites,
- adding a new UI framework,
- duplicating state already tracked elsewhere,
- making the menu more deeply nested,
- using long blocking delays.

If a proposed implementation differs from this document but achieves the same UX outcome with less complexity, use it and explain the decision.

---

## Expected Deliverables

For each implemented change, provide:

1. short description of the UX problem,
2. changed files,
3. implementation summary,
4. host-side tests added or updated,
5. compatibility/safety considerations,
6. any behavior intentionally left unchanged.

If some items are already solved in the current code, mark them as **already satisfied** instead of rewriting them.

---

## Definition of Done

The work is successful when the menu feels:

- immediate,
- stable,
- predictable,
- logically organized,
- tolerant of normal human input timing,
- clear about current state,
- clear about errors,
- easy to resume after interruption,
- free from avoidable blocking and visual lag.

The agent is encouraged to improve on the exact implementation details in this document, but should preserve these UX goals.
