# Building BACCAble

The supported build entry point is this directory's Makefile. Use GNU Make,
Clang for host tests, cppcheck, and the full Arm GNU Toolchain including
newlib-nano. Builds target Cortex-M0 with GNU C11 and LTO. The toolchain release
used by CI is pinned in [ci.yml](../../.github/workflows/ci.yml) (currently
15.3.Rel1); dated local measurements may use a different release.

From the repository root:

```sh
make -C tests test
make -C firmware/baccable FLAVOR=C1 lint
make -C firmware/baccable -j4 FLAVOR=C1 all
make -C firmware/baccable -j4 FLAVOR=C2 all
make -C firmware/baccable -j4 FLAVOR=BH all
make -C firmware/baccable -j4 FLAVOR=CAN all
```

Run lint for each of the four flavors as CI does. The default flavor is CAN.
Artifacts are `build/FLAVOR/baccable-FLAVOR.{elf,bin,hex,map}`. Objects and
header dependencies are isolated per flavor; changed flags trigger rebuilding.
Do not run parallel `make clean all`. A clean build is optional:

```sh
make -C firmware/baccable FLAVOR=C1 clean
make -C firmware/baccable -j4 FLAVOR=C1 all
```

A toolchain outside PATH can be selected explicitly:

```sh
make -C firmware/baccable FLAVOR=C1 TOOLCHAIN=/path/to/bin/arm-none-eabi- all
```

Use `EXTRA_CPPFLAGS`, not replacement `CFLAGS`, for custom preprocessor options:

```sh
make -C firmware/baccable FLAVOR=C1 VERSION=local-test EXTRA_CPPFLAGS="-DIS_GASOLINE -DIPC_MY23_IS_INSTALLED" all
```

Build options are in `app/build_config.h`; saved-option defaults are in
`settings/setup_entries.c`, with startup state in `state/`. A local
`Core/Inc/user_config.h` may use `user_config.h.sample` as a reference;
enable it with `EXTRA_CPPFLAGS=-DINCLUDE_USER_CONFIG_H`.
All connected boards must use the same display width.

`make FLAVOR=C1 flash` uses dfu-util to write the matching binary. Flashing is
never part of test/build verification. First review the storage migration and
hardware requirements in the [flashing guide](../../docs/FLASHING.md).

The C1 linker script allocates 96 KiB for the program; other flavors allocate
64 KiB. RAM is 16 KiB. The current C1 image, persistent records and USB disk need
128 KiB of physically reported Flash. C1's optional disk is 20 KiB; C2/BH disks
are 52 KiB. Saved records remain at `0x0801d000..0x0801ffff`.
Host tests do not certify vehicle timing or electrical behavior.

CI runs host tests, cppcheck and the four standard builds. Additional local
configurations and measured sizes are listed in the
[build report](../../docs/architecture/UPSTREAM_BUILD_SIZES.md). The C1 pedal
serial adapter now fits its allocation; earlier 64 KiB overflow reports are obsolete.

Stable releases use `build.yml` and a `vX.Y.Z` tag. Beta releases use
`build-unstable.yml`; its `CFLAGS` input is validated and forwarded as
`EXTRA_CPPFLAGS`. Both publish only after the shared CI workflow succeeds.
Published ELF names retain the `_stable`/`_beta` aliases; BIN/HEX names remain
`baccable-C1`, `baccable-C2`, `baccable-BH` and `baccable-CAN`.
