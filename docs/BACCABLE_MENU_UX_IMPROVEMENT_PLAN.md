# Menu UX delivery status — v5-beta-5

The implementation series is closed. This file preserves the original 20 item
numbers as a status record, not a new execution plan. Unfinished proposals live
in [MENU_UX_BACKLOG.md](MENU_UX_BACKLOG.md). Status reflects code and host tests;
vehicle validation is still outstanding.

| # | Original proposal | Status | Delivered behavior / remaining scope |
| --- | --- | --- | --- |
| 1 | Reduce display latency | Backlog | Latest-target dirty fragments already exist; pacing remains 50 ms. Faster pacing needs vehicle measurements. |
| 2 | Immediate action feedback | Partial | Immediate acknowledgement and diagnostic progress exist. Confirmed outcomes for every vehicle command need authoritative replies; see backlog. |
| 3 | Consistent status language | Delivered in scope | 4WD/QV request states and fault-clear WAIT clarified. Requests are not presented as confirmed vehicle states. |
| 4 | Revisit input timeout | Backlog | 300 ms stream-loss guard retained; boundary tests added. Measure report gaps before changing it. |
| 5 | Longer BACK hold | Delivered | 1200 ms; short SELECT and held BACK remain mutually exclusive. |
| 6 | Hold-to-repeat navigation | Delivered | 500 ms delay, then 180 ms intervals on fresh reports, navigation allowlist, no action/group/SELECT/BACK repeat or catch-up bursts. |
| 7 | Inactivity auto-close | Delivered | 30 s navigation, 60 s settings/editors; persistent readings, save-error and fault-operation guards, retryable display clear. |
| 8 | Avoid unchanged Flash writes | Already satisfied + tested | Record payload deduplication already prevents erase/program. Added interrupted-save regression coverage; board synchronization remains unchanged. |
| 9 | Preserve navigation context | Already satisfied + tested | Actions/Settings/Information retain RAM positions, including close/reopen; unavailable remembered actions are skipped. |
| 10 | Fewer steps for frequent actions | Partial / optional backlog | Favorites, root ordering and remembered Actions already help. Pinned actions are deferred. |
| 11 | Separate actions from setup | Already satisfied | Actions execute commands; Feature setup controls availability. |
| 12 | Diagnostic error reasons | Delivered | Timeout, ECU rejection, invalid response and send deadline failure, with retry recovery tests. |
| 13 | Show stale/missing data | Already satisfied + tested | Cache expiry renders missing values as `--`; no new source-specific issue established. |
| 14 | Keep technical details in Information | Already satisfied | Information and Advanced pages contain technical content. |
| 15 | Overall system health | Backlog | Individual peer information remains; optional-board and compatibility rules are needed for an aggregate status. |
| 16 | Consistent symbols | Delivered in scope | ASCII `+ - * > < ! ?` meanings documented; requested states retain explicit wording. |
| 17 | Different feedback durations | Delivered | Simple toggle/save notices 750 ms, warnings 1800 ms, other request notices 1200 ms; diagnostic progress follows its state machine. |
| 18 | Nonblocking cosmetic feedback | Preserved + tested | Navigation dismisses notices; confirmations and vehicle guards remain. |
| 19 | Rotation respects navigation | Already satisfied + tested | Manual selection restarts the 5-second rotation period. |
| 20 | UX regression tests | Delivered | 60 reported scenarios across 11 host executables, both display widths, ASan/UBSan; Actions summary and downloadable HTML report. |

## Delivery and validation

PRs #8–#11 delivered the initial UX regression/feedback series; #12 added Actions
reports; #13 added repeat, idle closure and notice durations. The release workflow
runs host/script tests, static analysis, C1/C2/BH/CAN builds and size gates before
publication. Runtime changes preserve page IDs, Favorites and settings formats.

Vehicle checks still needed: short/held RES, repeat feel, editor timeout and
save-error recovery, factory display/radio handover. This series does not establish
that the previously reported device freeze is resolved or make fragmented screen
updates atomic.

See [menu behavior](architecture/MENU_UX.md), [test reporting](../tests/README.md)
and [flashing](FLASHING.md). Do not reopen completed tasks without a reproducible
regression; keep future patches small and attach focused tests.
