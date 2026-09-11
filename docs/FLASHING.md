# Flashing the current firmware

Use images from the same release or local build, with matching display widths.
The current C1 program exceeds 64 KiB and requires **physically confirmed 128 KiB
Flash**. A C8 marking or a large ELF file is not a capacity measurement. Review
[storage compatibility](architecture/README.md#persistent-data-and-compatibility)
and retain needed settings/files before an upgrade from older firmware.

## Select the board and image

| Board / USB port label | Role | Local or release binary |
| --- | --- | --- |
| C1 | Powertrain, main menu, ELM gateway | `baccable-C1.bin` |
| C2 | Chassis | `baccable-C2.bin` |
| BH | Body and dashboard | `baccable-BH.bin` |
| Standalone CAN adapter | SLCAN | `baccable-CAN.bin` |

Prefer PCB labels over left/right descriptions. Each USB port programs its own
controller. CAN firmware is a separate adapter role, not a replacement for one
member of the C1/C2/BH vehicle set.

Download assets from [this fork's releases](https://github.com/eujot/BACCAble/releases), or build with the
[Makefile](../firmware/baccable/MAKEFILE.md). Local output is under
`firmware/baccable/build/FLAVOR/`. Check the release commit and flags: an older
release may not include the current source changes. ELF assets retain legacy
names such as `baccableC1_beta.elf`; this procedure uses the `.bin` file.

## Write with dfu-util

1. Put the target controller into its STM32 DFU bootloader using the procedure for
   that PCB revision. A runtime USB disk or serial port is not DFU mode.
2. Connect that controller's USB port. Run `dfu-util --list` and confirm that the
   intended target appears as `0483:df11`, with internal Flash at alternate 0.
   Keep only one DFU target connected while using the command below.
3. Write the matching binary. For C1, from the repository root:

   ```sh
   dfu-util -d 0483:df11 -c 1 -i 0 -a 0 -s 0x08000000:leave -D firmware/baccable/build/C1/baccable-C1.bin
   ```

4. Require successful completion, restart the controller, then repeat with the
   C2 and BH ports and their respective files. Update the complete board set.
5. Confirm firmware versions in the menu's Information view, display width/engine
   settings and communication between boards. Restore calibration if required by
   the upgrade path. Test changed vehicle behavior on the actual hardware.

The Makefile's `flash` target performs a build and the same download operation;
it does not select the physical board or verify its Flash capacity for you.
