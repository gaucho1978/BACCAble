# Memory optimization snapshot

Measured locally on 2026-09-10 with ARM GNU Toolchain 15.2.rel1 and
`VERSION=menu-ux`. This is the stage **before final cleanup**; current results
are in [cleanup validation](CLEANUP.md). The baseline already had English labels
and a single settings-screen buffer. These changes preserved page counts,
measurement decoding, queue capacity and persisted record formats.

## Changes

1. Reordered `ParameterDefinition` fields from 40 to 36 bytes, saving 400 bytes
   across 100 definitions. Natural alignment is preserved without `packed`.
   These named-field initializers are not serialized records. Final cleanup
   subsequently removed unused presentation metadata, reducing each entry to
   28 bytes; templates still supply the displayed units and precision.
2. Reduced the CAN TX queue from 900 to 452 bytes. Each entry stores one ID,
   DLC, IDE/RTR/global-time flags and eight data bytes. The 28-slot ring still
   holds 27 pending messages and preserves FIFO, copied data and retry behavior.
   The full 24-byte HAL header is created temporarily on the stack. The supplied
   HAL reads it synchronously in `HAL_CAN_AddTxMessage` without retaining its
   address. Standard/extended IDs, data/remote frames and global time remain supported.
3. Reduced cache-validity flags from 100 to 13 bytes using one bit per reading.
   Values and 32-bit timestamps remain unchanged. NaN/Inf invalidate a reading;
   expiry remains three seconds with clock-wrap handling.

## Build comparison before cleanup

Flash is `text + data`; RAM is `data + bss`, in bytes. Linker alignment and LTO
mean whole-image differences need not equal the sum of individual structures.

| Variant | RAM before → after | RAM saved | Flash before → after |
| --- | ---: | ---: | ---: |
| C1, 18 characters | 9964 → 9436 | 528 | 64816 → 64184 |
| C1, 24 characters + gasoline + MY23 + LED | 10172 → 9636 | 536 | 64792 → 64332 |
| C2, 18 characters | 10728 → 10280 | 448 | 22216 → 22232 |
| BH, 18 characters | 10708 → 10260 | 448 | 23212 → 23232 |
| CAN | 7104 → 6656 | 448 | 21428 → 21408 |

C1 recovered 632/460 bytes of Flash, leaving 1352/1204 bytes in the 64 KiB
program region for 18/24-character builds. C2/BH exchanged 16/20 bytes of extra
code for 448 bytes less RAM. `data + bss` includes the linker's heap/stack
reservation; it does not measure peak runtime stack usage.

## Validation of this stage

Six host executables passed with ASan/UBSan. CAN tests cover repeated ring wraps,
ordering, DLC 0–8, both ID formats, RTR, global time and HAL_BUSY/HAL_ERROR retry.
Cache tests cover byte boundaries, parameter 99, neighboring validity bits, reset,
invalid indices and timestamp wrap. Catalog tests check structure size and references.
Baseline builds and cppcheck passed for C1/C2/BH/CAN; 24-character builds also
passed for C1/C2/BH, including gasoline, MY23 and LED options on C1.

The large C1 LED PWM buffer remains: reducing it requires changing DMA operation,
not simply shrinking an array. These optimizations do not change DMA or bus timing.
This stage was not flashed to a device or verified by remote CI.
