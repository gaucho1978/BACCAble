# Idle home and verified IPC glyphs

Requirements: [Idle Timeout and Latin-1 UI Plan](../BACCABLE_IDLE_TIMEOUT_AND_LATIN1_UI_PLAN.md).

## Idle behavior

The previous idle path saved, hid the menu and sent spaces. Hidden-menu handling
intentionally ignores NEXT/PREV until BACK reopens Favorites; this explains the
reported apparent input lockout. Fragmented blank transmission can expose partial
old content. That diagnosis does not establish the cause of unrelated device freezes.

Idle now cancels local drafts/capture, persists committed changes and opens the
remembered Favorites without hiding or clearing the overlay. Normal browse timeout
is still 30 s; configuration is 60 s. Active fault read/clear keeps its existing
protection. Vehicle-request sequences are not cancelled or altered by returning home.
An empty favorites list stays visible as No favorites; BACK still reaches ROOT.

Failures retain the existing explicit retry state and the intended destination.
SELECT retries an idle return to Favorites; it does not close the menu. ROOT/BACK
is still explicit close and retains its UART-clear retry behavior.

The 0x11 clear code appears in body/display-frame comments, but there is no proven
factory-radio ownership handoff here. No new release command or diagnostic CAN
experiment was added. The idle fix does not depend on changing that protocol.

## Byte vocabulary

The supplied plan reports physical IPC verification of printable ASCII 0x20–0x7E
and Latin-1-like 0xA0–0xFF. The 0x80–0x9F range is unsupported/control, not CP1252
punctuation. These are user-reported hardware findings; this change does not claim
new physical validation or identical rendering on every future IPC variant.

| Byte | Meaning / use |
| --- | --- |
| 4F / D8 | O / Ø: editable boolean and favorite/visibility membership |
| 2A | *: temporary edit or reorder selection |
| 3E | >: submenu or explicit action |
| 21 / 3F | ! attention/precondition; ? unknown or absent vehicle confirmation |
| D7 | ×: failed save/read/request attempt, not inferred physical vehicle failure |
| AB / BB | « / »: numeric editing directions, only when the full label/value fits |
| B0 | °: Celsius unit, using an existing gap or spare space |
| B7 / B1 | · / ±: verified and reserved; no artificial separator/tolerance feature added |

`ui_glyphs.h` is the byte source of truth. Firmware strings use numeric constants
or escaped single-byte strings, never UTF-8 glyph literals. ON/OFF and WAIT remain
for enums, vehicle requests and status-only information. The raw diagnostic sweep
is retained, including unsupported candidate bytes for testing other IPCs.

## Deliberate refinements

- Reuse the existing saved-exit destination instead of creating a second persistence
  path. Both idle retry and explicit-close retry retain their original intent.
- Use a dedicated checkbox renderer so the migration cannot turn vehicle state or
  enum values into checkbox claims.
- Preserve all original numeric fields before adding optional degree/direction
  glyphs. Reusing a Celsius unit gap gives a compact number°C without added width.
- Keep temporary queue-busy/precondition warnings distinct from failed operations.
  A missing Dyno/brake confirmation is unknown; it does not prove physical failure.

## Validation

Host regressions cover all ordinary/editor idle contexts, immediate NEXT/PREV,
committed-setting persistence, draft/capture cancellation, failed-save retry,
explicit-close retry and absence of blank screens on idle. Byte-level tests cover
checkbox UART output, padding, editor capacity guards, degree insertion, preserved
dense fields and production-byte validation across both catalogs.

Run the existing full test target for 18/24-character and diagnostic suites.
Production builds and size gates remain in CI. Physical follow-up should confirm
idle return and the adopted glyphs on the target IPC; manual radio/RDS handoff and
other dashboard revisions still need their own hardware verification.
