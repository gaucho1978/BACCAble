# Upstream integration build sizes

Measured locally on 2026-09-11 with ARM GCC 15.2.Rel1, `-Os`, LTO and
`VERSION=upstream-sync`, for the firmware integrated in `b29102a`. These are
a dated baseline, not measurements of every later commit. Values are bytes. `text + data` measures the linked Flash
image; `data + bss` includes the linker's 512-byte heap and 1024-byte minimum stack
reservation. It is not a measured runtime stack high-water mark. The binary size
can include alignment padding. ELF debug information is not flashed.

| Build directory | Flash (`text + data`) | RAM and reservations (`data + bss`) | Binary file |
| --- | ---: | ---: | ---: |
| C1 | 83384 | 14512 | 83384 |
| C2 | 28072 | 12036 | 28072 |
| BH | 29180 | 12032 | 29180 |
| CAN | 24260 | 6824 | 24264 |
| validation-c1-large | 83328 | 14736 | 83332 |
| validation-c2-large | 28140 | 12164 | 28140 |
| validation-bh-large | 29164 | 12168 | 29164 |
| validation-c1-ucan | 83688 | 14516 | 83692 |
| validation-c1-pedal | 74496 | 13524 | 74496 |
| validation-bh-debug | 24244 | 7792 | 24244 |

C1's allocation is 98304 bytes of program Flash; the other flavors allocate
65536 bytes. All variants have 16384 bytes of RAM. Record storage requires the
additional Flash space described in [the integration report](UPSTREAM_SYNC.md).

Additional build flags:

| Directory suffix | Flags |
| --- | --- |
| c1-large | `-DLARGE_DISPLAY -DIPC_MY23_IS_INSTALLED -DIS_GASOLINE -DLED_STRIP_CONTROLLER_ENABLED` |
| c2-large / bh-large | `-DLARGE_DISPLAY` |
| c1-ucan | `-DHSE_ENABLED_FOR_UCAN -DUCAN_POWER_PINS` |
| c1-pedal | `-DACT_AS_SCHIZZAFORTE_SERIAL_CONTROLLER` |
| bh-debug | `-DDEBUG_MODE` |

Run `make -C tests test` for host regressions. Run `make -C firmware/baccable
FLAVOR=C1 lint` and `make -C firmware/baccable FLAVOR=C1 all` for standard firmware
validation, substituting each flavor. To reproduce an additional variant, pass
its flags as `EXTRA_CPPFLAGS` and a separate `BUILD_DIR=build/validation-NAME`.
The host tests do not replace tests with actual ECUs and USB hosts.
