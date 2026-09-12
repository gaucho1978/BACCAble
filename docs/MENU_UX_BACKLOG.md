# Menu UX backlog

Deferred from the [v5-beta-5 delivery status](BACCABLE_MENU_UX_IMPROVEMENT_PLAN.md).
These proposals are not release blockers and are not scheduled for implementation.
Original plan numbers are preserved for traceability.

| Priority | Item | Next evidence / acceptance boundary |
| --- | --- | --- |
| 1 | #1: faster dashboard updates | Capture the existing 50 ms behavior and CAN traffic; compare a separate 30 ms build in the vehicle. Preserve retries/fairness and check factory-radio handover. Host tests cannot certify dashboard tolerance or atomic redraw. |
| 1 | #4: more tolerant input-stream timeout | Measure report gaps during normal/slow presses and stream loss. Adjust 300 ms only if evidence supports it; a lost release must not become SELECT. Keep boundary/wrap tests. |
| 2 | #2: confirmed outcome for every vehicle action | Identify authoritative ECU/peer acknowledgement for one action at a time. Distinguish requested, pending, confirmed and failed states without assuming successful execution from queue acceptance. |
| 3 | #15: aggregate system health | Define required versus optional boards, freshness and compatible versions first. Never report missing optional hardware as a fault or stale version data as SYSTEM OK. |
| 4 | #10: pinned/favorite actions | Establish a frequent task that existing Favorites and remembered Actions do not serve. Decide whether it justifies new preference semantics; preserve page IDs and existing Favorites. |

Each task gets one bounded patch and focused host regressions. Start only after its
required evidence is available. No automatic execution of vehicle-control actions,
broad menu refactor, guessed protocol values or new persistence format merely for
cosmetic improvements.

Before claiming the delivered UX hardware-validated, also check repeat feel,
RES/BACK, inactivity saves and display handover in the vehicle. This validation is
separate from implementing the deferred features above.
