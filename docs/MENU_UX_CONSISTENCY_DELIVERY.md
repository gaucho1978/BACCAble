# Menu consistency delivery and acceptance

Requirements: [Menu UX Consistency Refactor Plan](BACCAble%20Menu%20UX%20Consistency%20Refactor%20Plan.md).
Baseline: v5-beta-6. Automatic persistence was delivered in PR #16; this follow-up
completes the remaining software behavior. Physical dashboard checks remain separate.
The original requirements document is retained unchanged.

## Delivered requirements

| Plan phases | Result |
| --- | --- |
| 0–1 | Baseline inventory and a tested global interaction contract; current per-view table in [MENU_UX.md](architecture/MENU_UX.md). |
| 2–7 | Both Save rows removed. Exact serialized comparison detects committed changes, including changes outside callbacks. Independent settings/preferences saves skip unchanged data, board sync and USB apply. Success is silent; SELECT retries the remembered failed exit; BACK cancels that exit without discarding RAM changes. |
| 8–9 | BACK unwinds one level or cancels a nested draft/capture. Reading, favorite and normal information SELECT is a no-op. No SELECT event is rewritten to BACK. |
| 10–18 | Shared bounded position rendering across Root, Groups, Settings, Actions, Information, Features and the nested Park Mirror list. Actual visible peers determine totals; temporarily unavailable actions still count. |
| 19–21 | Reading/favorite positions, membership/visibility/order lists and fault-result positions. Existing filtering supplies counts. Empty lists and fault progress/failure have no artificial position. |
| 22–25 | Existing request/confirmation semantics, entry types and hold-repeat restrictions retained. Idle cancels unaccepted drafts, persists committed data and closes only after successful persistence. |
| 26–29 | Source transition/save audit and host contracts cover all views, visible counts, nested cancellation, save failures and 18/24-byte bounds. Existing setup/page filters are reused; setup exposes a small position query for nested lists. |
| 30 | Cleanup reviewed. No action-module extraction: it is optional and adds no behavior needed for this delivery. No new UI framework, allocation, storage format or CAN protocol. |

## Width policy and explicit exceptions

Counters never justify dropping a measurement, unit, unknown marker or version
character. Short readings keep `x/y` beside the complete text. If the original
reading cannot fit with its counter, a numbered page title appears for 1200 ms
after selection, then the complete original reading uses the full screen. The
same rule protects long firmware-version strings. This is an intentional
exception to a **continuously visible** counter, necessary for dense 18-character
pages; it preserves existing page IDs and multi-value layouts. Automatic rotation
also shows the title. Query cadence and the 50 ms transport interval are unchanged.

Setup values take priority over shortened labels. Warning rows may remove spaces
around the warning symbol to retain the entire condition. Compact English labels
include Features, Favorites, Shown pages, Fav. order, BCM faults, Clear DTCs,
Reset times, Peak hold, End launch and IPC diag. These labels do not rename IDs or
change the operations they invoke.

Notices, save errors, pending/completed request messages, numeric drafts, capture
instructions and raw charset ranges are not peer-list screens and are unnumbered.
A successful Dyno/brake reply restores the numbered action state. An explicit
`< Back` row in Park Mirror is actionable; no status page silently goes backward.
Physical BACK still opens Favorites when the menu is closed.

## Persistence and baseline findings

The baseline had explicit Save rows, multiple exit-save paths, success notices,
SELECT-to-BACK retry rewriting and SELECT-to-ROOT status pages. PR #16 replaced
persistence with independent saved-value comparisons. Settings reuse their cache;
preferences retain an 80-byte snapshot. Missing/invalid records, defaults and
migrated preferences remain unsaved until the first successful exit.

Only successful saves update snapshots. A preferences-only change does not restart
board synchronization or apply USB. Partial success does not repeat the successful
domain. Failed exits remain modal until SELECT retries or BACK cancels the exit;
idle processing does not repeatedly retry flash. The two records remain separate,
not an atomic transaction. Power loss before persistence can lose unsaved RAM changes.

## Audit and regression evidence

The audit covered MenuView transitions, MENU_SELECT/MENU_BACK, save calls,
setup_back, notice/retry paths, display formatting and fault-result rendering.
The fault-result SELECT path now also checks Clear before restarting a read;
previously only entering the reader from Actions checked this condition.

Existing sanitizer tests are extended, not replaced. `test_menu_contract.c` covers:

- Every section's parent, status SELECT no-op, nested numeric/Park cancellation,
  diagnostic return and fault-workflow return.
- Visible action/setup counts, unavailable actions, filtered favorite/editor totals,
  reorder position and retention of engine-incompatible saved favorite IDs.
- Every supported gasoline/diesel page template, with ordinary and unavailable
  values: the complete original reading survives numbering at 18/24 characters.
- Bounded rendering, long-version protection, stale peer versions and fault codes.

The previous persistence, queue retry, request timeout, mirror capture and repeat
regressions remain in the host suite. CI exposes the additional cases in its
existing summary and HTML artifact. Release readiness requires passing all host
suites, production C1/C2/BH/CAN builds, static analysis and size gates.

## Hardware acceptance

Still verify rapid browsing, the 1200 ms dense-page title, readability of shortened
labels, both real display widths and vehicle action feedback on an actual IPC.
Host tests do not certify physical dashboard rendering. Extended glyph approval,
CAN timing changes and investigation of the previously reported device freeze are
outside this consistency refactor.
