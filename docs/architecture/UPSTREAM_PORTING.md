# Porting future upstream changes

For developers and AI agents integrating [gaucho1978](https://github.com/gaucho1978/BACCAble)
or [netzmark](https://github.com/netzmark/BACCAble). Port behavior into the current
modules. Do not replace them with upstream files or blindly merge/cherry-pick
commits from the monolithic `firmware/ledsStripController` tree.

## 1. Establish the comparison range

Start from the current local integration, preserving uncommitted work. Read
[UPSTREAM_SYNC.md](UPSTREAM_SYNC.md) for the last reviewed source SHAs, feature
mapping and deliberate exclusions. Those SHAs are the comparison baseline;
`git merge-base` alone cannot identify functions already ported manually.

Fetch source history without changing the working tree or importing release tags:

```sh
git fetch --no-tags https://github.com/gaucho1978/BACCAble.git master:refs/remotes/upstream-gaucho/master
git fetch --no-tags https://github.com/netzmark/BACCAble.git master:refs/remotes/upstream-netzmark/master
```

For each source, use its recorded SHA as `BASE` and resolve the fetched head to an
immutable `HEAD_SHA`. Inspect `git log --reverse BASE..HEAD_SHA` and
`git diff BASE HEAD_SHA`. Check relevant stable branches/tags separately; do not
assume they match master. If history was rewritten, compare snapshots and find a
verified baseline before treating changes as new.

Classify each functional change as **already covered**, **port**, or **exclude
with a reason**. Compare both sources: a later commit date does not make its
implementation correct. Retain local fixes and document conflicting behavior.

## 2. Put each change in its owning module

Paths below are relative to `firmware/baccable`.

| Change | Destination and wiring |
| --- | --- |
| Vehicle signal or CAN handler | `vehicle/*_frames.c`; declare in `frame_handlers.h`, register in `standard_frames.c` or `diagnostic_frames.c`; update the relevant `state/*` domain. |
| Reading or diagnostic parameter | `diagnostics/parameter_catalog.c`; native values in `native_parameters.c`, freshness/maxima in `parameter_cache.c`, requests in `parameter_request.c`, decoding in `uds_decode.c`. |
| Menu page, action or presentation | Catalog for pages; `features/menu.c` for actions/navigation, `menu_model.c` for preferences, `dashboard_format.c` for text. Keep `display_stream.c` as the display transport. |
| Periodic vehicle behavior | Corresponding `features/*.c` module; schedule through `app/powertrain.c` (C1), `features/chassis.c` (C2), or `features/body.c` (BH). Keep `app/main.c` focused on scheduling. |
| Parking, pedal, faults | Extend `features/parking.c`, `parking_mirrors.c`, `pedal_map.c`, `pedal_booster.c`, or `diagnostics/fault_reader.c`; avoid parallel implementations. |
| Saved option / board command | `state/settings.h`, `settings/setup_entries.c`, `setup_menu.c`, `persistence.c`; sender in `features/board_sync.c`, receiver in `app/board_commands.c`, command definitions in `transport/board_uart.h`. |
| ELM, capture or communication | `protocol/elm327.c`, `transport/diagnostic_link.c`, `features/usb_modes.c`; shared queues in `transport`, USB class lifecycle in `USB_DEVICE`. |
| Hardware / build option | `platform/*`, `app/build_config.h` and `Makefile`. Keep board-specific pins and clocks conditional. Do not import generated CubeIDE output. |

For each port, follow the complete path: **input → state → feature → output**,
including settings, startup, timeout, disable and wake-up behavior. Preserve
upstream attribution and add short English comments describing functional purpose.

## 3. Preserve these compatibility contracts

- **Identifiers and storage:** keep existing page IDs, parameter meanings, group
  IDs, setting slots and board commands. Remap colliding upstream IDs. Preserve
  page-table order: visibility bits use table indices. Add pages at the end.
- **Capacity:** gasoline already uses all **64 page slots**. Adding a 65th page
  requires coordinated catalog/list expansion and migration of the 64-bit
  visibility representation. Do not just increase the page count. Also check
  the 100-entry parameter/cache limit and four-value rendering/request limit.
- **Saved data:** retain the 40-slot settings record `0x101`, menu record `0x104`
  and existing record addresses. Layout changes require a new record identity,
  explicit migration and tests loading old records. Never silently reinterpret
  saved bytes. Keep existing user preferences ahead of new defaults.
- **Menu:** retain favorites, sorting, visibility, input gestures, live status
  values and stale-value handling. Test English labels and rendered values at
  both 18 and 24 characters; do not copy upstream display indices or templates
  without adapting them. See [the menu guide](MENU_UX.md).
- **Communication:** validate CAN ID/type/DLC and diagnostic ECU/DID/sequence.
  Retain owned buffers, bounded work, retries on busy queues and timeout cleanup.
  Keep interrupt handlers limited to collection/completion; no feature logic or
  Flash writes there. Preserve diagnostic exclusivity and USB/LED pin ownership.
- **Vehicle behavior:** preserve fresh-input checks, manual overrides and restore
  only changes owned by the feature. Keep optional additions off by default and
  inactive experiments inactive unless explicitly implemented and validated.
- **Hardware:** C1 allocates 96 KiB program Flash; other flavors 64 KiB; RAM is
  16 KiB. C1 requires physically confirmed 128 KiB Flash. Preserve disk/record
  boundaries in both linker scripts and `storage/flash_layout.h`. Keep C1/C2/BH
  firmware and display widths matched; do not imply mixed-version support.

## 4. Validate and record the integration

Add focused regressions for changed behavior and failure paths in `tests/`, then:

```sh
make -C tests test
for flavor in C1 C2 BH CAN; do
    make -C firmware/baccable FLAVOR="$flavor" lint all || exit 1
done
git diff --check
```

Build affected optional configurations from [UPSTREAM_BUILD_SIZES.md](UPSTREAM_BUILD_SIZES.md),
including large display/MY23 when menus change. Compare linked Flash/RAM usage
and reservations, not ELF file size. Follow `.clang-format`; preserve vendor code.
Record hardware checks still needed for changed timing or actuator behavior.

Update `UPSTREAM_SYNC.md` with exact source ranges, feature-to-module mapping,
exclusions and validation results; update size/usage docs when affected. Record
the local integration commit in the PR. Advance the reviewed source baseline
only when every change in that range is accounted for; carry unfinished items
forward explicitly. A successful build alone does not prove vehicle compatibility.
