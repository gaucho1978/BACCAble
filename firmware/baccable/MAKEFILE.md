# Building BACCAble

The supported build entry point is this directory's Makefile. Use GNU Make,
Clang for host tests, cppcheck, and the full Arm GNU Toolchain including
newlib-nano. CI uses Arm GNU Toolchain 15.2.rel1, Cortex-M0, GNU C11 and LTO.

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

Options and defaults are in `app/build_config.h`. A local
`Core/Inc/user_config.h` may use `user_config.h.sample` as a reference;
enable it with `EXTRA_CPPFLAGS=-DINCLUDE_USER_CONFIG_H`.
All connected boards must use the same display width.

`make FLAVOR=C1 flash` uses dfu-util to write the matching binary. Flashing is
never part of test/build verification. First review the storage migration and
hardware requirements in [architecture notes](../../docs/architecture/README.md).

The firmware link script limits code to 64 KiB and RAM to 16 KiB.
Persistent records and the USB disk need 128 KiB of physically reported Flash.
Host tests do not certify vehicle timing or electrical behavior.
