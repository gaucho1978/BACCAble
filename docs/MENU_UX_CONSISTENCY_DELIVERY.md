# Menu consistency delivery

Scope: [requirements](BACCAble%20Menu%20UX%20Consistency%20Refactor%20Plan.md), based on v5-beta-6 / master `1dc0595`. This is a staged delivery plan, not a claim that the new behavior is implemented.

## Initial findings

- `menu.c:save_all()` invokes settings and preference persistence, shows Saved, and short-circuits preference saving when settings saving fails.
- ROOT/SETUP exits and idle close use that helper; preference editors have separate save paths. SETTINGS has Save; SETUP has a synthetic Save and back page.
- Save retry rewrites SELECT to BACK through `retry_back`.
- SELECT on readings/favorites returns to ROOT. Normal information pages also need the new status/no-op contract.
- `settings_save()` restarts board synchronization and applies USB after a successful record save. Menu-level unchanged visits must avoid those side effects, even if the storage layer already avoids identical writes.
- Numeric drafts, explicit mirror capture, request feedback and repeat protection already exist. Preserve them.

## Ordered deliveries

| Order | Scope | Completion gate |
| --- | --- | --- |
| 1 — Reliable automatic persistence | Finish per-view baseline and intended transition table; add behavioral tests, settings/preferences change tracking, one persistence-on-exit path and explicit retry state. Remove both Save entries only together with their replacement. Success is silent; unfinished drafts/capture remain cancelled on exit or idle. | No-change visits cause no save/sync/USB apply. Only changed domains save. Failure retains unsaved state; explicit retry resumes the intended exit. Partial success does not repeat the successful domain. Numeric cancel and capture cancel do not commit. |
| 2 — Navigation consistency | Normalize one-level BACK and status SELECT no-op across all views. Preserve actionable diagnostics, fault operations, submenu entry, physical menu opening and safe repeat. | Table-driven transitions for every view, including nested Park/editor/diagnostic states, empty lists and save-error cancellation. No hidden SELECT-to-BACK rewriting. |
| 3 — Compact menu numbering | Add one bounded position renderer and visible-position helpers; cover Root, Groups, Settings, Information, Actions, Setup and nested browsable menus. | 18/24-character cases, filtered counts, first/last positions, preserved ON/OFF and mode values; no numbering on pending/error/confirmation screens. |
| 4 — Readings and remaining lists | Apply numbering to readings, favorites, membership/order editors and actual fault results. Reuse real filters and quality formatting. | Long/multi-value readings retain values, units and stale/no-data markers; correct engine/favorite counts; no counters on fault progress/failure or charset ranges. |
| 5 — Acceptance and release readiness | Search for remaining navigation/save exceptions, update user docs, run full host/CI gates and document physical checks. | Four production flavors, 18/24 host suites and diagnostic integration pass; storage IDs and request semantics unchanged. Hardware checks reported separately. |

Stage 1 is the first implementation task. Within it, establish tests before changes and land only a passing, complete persistence transition. Stage 2 must follow promptly so the eventual release has one navigation contract. Do not publish a final consistency release before stages 1–5 pass.

## Small work units and resource limits

- Use the existing host fixtures and sanitizer runner; add behavioral cases rather than another test framework.
- One implementation owner at a time for `menu.c` and setup/persistence integration. Do not give overlapping edits to multiple agents.
- If delegation is used, assign a bounded read-only audit or an isolated renderer/test task after its API is agreed. Pass only relevant files, expected behavior and acceptance cases.
- Review only the changed paths during each stage. Run focused tests while editing; full checks once per completed delivery, repeating only after relevant changes or failures.
- Keep numbering out of the first two deliveries. Do not retune CAN/display timing or redesign vehicle commands, entry types, storage or physical input.
- `menu_actions.c` extraction is optional cleanup after acceptance, not a release dependency. Defer it unless the completed changes demonstrate a concrete need.

## Decisions required during implementation

- Inventory all mutation paths, including remembered pages, automatic rotation, external setting changes and startup defaults. Track committed serialized values or equivalent domain revisions; a UI callback-only flag must not miss changes made elsewhere.
- Define the save-error destination explicitly: SELECT retries the pending persistence/exit; BACK cancels the attempted exit and stays in the current configuration context without discarding committed RAM changes. Avoid recursive event rewriting and automatic repeated flash attempts.
- Keep successfully saved domains clean if another domain fails. Board synchronization and USB application must match successful settings persistence, not preference-only changes.
- Distinguish visible actions from temporarily unavailable actions: an unavailable but browsable action still belongs in the denominator.
- Numbering must not silently drop measurements. If a data-heavy page cannot fit all critical values and its counter, resolve its layout with 18-character tests before marking stage 4 complete.

## Status

Stage 1 implementation and baseline inventory are complete on this branch; verification and PR status are reported in the PR. Stages 2–5 remain pending. The original requirements document is unchanged.

## Stage 1 implementation

Automatic persistence replaces both Save entries. Dirty state is derived from exact
serialized comparisons against the last successfully stored values, rather than
flags scattered among callbacks. Settings reuse their existing cache; preferences
add an 80-byte snapshot. A missing/invalid record remains unsaved until the first
successful exit, including default or migrated preferences. Runtime mutations and
remembered page changes are detected without changing storage IDs or formats.

Both domains are attempted independently. Only a successful domain updates its
snapshot; unchanged settings skip flash, board synchronization and USB application.
On error, SELECT retries the original destination; BACK cancels the attempted exit
and retains RAM changes. Idle retries and gesture repeat are disabled in the error
state. Stage 2 status-page navigation and all new numbering remain pending.

### Per-view baseline at beta 6

NEXT/PREV browses peer items unless stated. The target navigation table is in the
original requirements; this table records the baseline, not the final contract.

| View | SELECT | BACK / cancellation | Persistence / explicit Save | Numbering |
| --- | --- | --- | --- | --- |
| ROOT | Enter submenu | Close | Both domains on close | Yes |
| FAVORITES | ROOT | ROOT | Remembered page saved on later exit | No |
| GROUPS | VALUES | ROOT | None directly | Yes |
| VALUES | ROOT | GROUPS | Remembered page saved on later exit | No |
| ACTIONS | Run/confirm action | ROOT | Action-specific records only | No |
| SETTINGS | Enter/change/Save | ROOT | Explicit Save; idle saves both | No |
| SETUP | Toggle/cycle/edit/capture/Save and back | Cancel nested workflow or SETTINGS | Both on exit; explicit synthetic Save item | No |
| EDIT_FAVORITES | Toggle membership | SETTINGS | Preferences on exit | No |
| EDIT_VISIBLE | Toggle visibility | SETTINGS | Preferences on exit | No |
| ORDER_FAVORITES | Pick/drop; NEXT/PREV reorders when picked | SETTINGS | Preferences on exit | No |
| INFORMATION | ROOT, or diagnostic entry when built | ROOT | None directly | No |
| FAULTS | Restart read | Cancel read, ACTIONS | None | Existing result index |
| DIAGNOSTICS | Switch character/pattern test | INFORMATION | None | Raw byte ranges |
| Numeric draft | Accept draft | Cancel draft, remain SETUP | RAM commit only | Not a peer list |
| Park submenu/capture | Enable, confirm capture, or explicit Back | Cancel capture or return to SETUP | Enable persists on later exit; capture queues vehicle command | No |

Stage 1 tests exercise unchanged visits, preference-only saves, both partial-failure
orders, repeated failure, explicit retry, cancellation, idle close and numeric
commit/cancel. Existing mirror capture regressions and the 18/24/debug suites remain
required. Subsequent deliveries must not reintroduce explicit Save or event rewriting.
