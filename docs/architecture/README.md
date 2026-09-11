# BACCAble firmware architecture

Start with the [build instructions](../../firmware/baccable/MAKEFILE.md),
[menu and extension guide](MENU_UX.md), and [development constraints](DEVELOPMENT.md).
The [integration report](UPSTREAM_SYNC.md) records source revisions and validation;
[build measurements](UPSTREAM_BUILD_SIZES.md) identify their date and configuration.
For gaucho/netzmark updates, follow the [upstream porting guide](UPSTREAM_PORTING.md).
For installing images, use the [flashing guide](../FLASHING.md).

## Responsibilities

Application code lives in `firmware/baccable`. Each variant is a single C program
with one event loop and statically allocated state.

| Directory | Responsibility |
| --- | --- |
| `app` | Startup, loop scheduling, build configuration and inter-board commands |
| `transport` | CAN/UART queues and communication, without vehicle feature decoding |
| `protocol` | SLCAN, ELM command/transaction handling and vehicle-frame checksums |
| `vehicle` | CAN-ID dispatch and vehicle reports grouped by function |
| `diagnostics` | Parameter catalog, local readings, UDS cache/validation and BCM fault reading |
| `features` | Periodic vehicle behavior, display rendering and menu navigation |
| `settings` | Feature definitions, preference changes and serialization |
| `state` | Named runtime state for telemetry, chassis, comfort, mirrors and other domains |
| `storage` | Recoverable records, Flash layout and the FatFs block device |
| `platform` | Power, GPIO, LEDs, clocks, interrupts and STM32 integration |
| `USB_DEVICE` | CDC/MSC integration with the ST USB library |
| `third_party` | FatFs and printf, kept separate from application code |
| `Drivers`, `Middlewares` | Supplied ST/CMSIS libraries |

## Execution and ownership

`main` initializes the platform and enabled features. Each loop processes a bounded
batch of CAN traffic, received UART messages, periodic features, USB, outgoing CAN
and status LEDs. Frame handlers receive a header and payload and check the required
length before accessing data. [Inactive signal notes](REFERENCE_SIGNALS.md) are
kept outside executable code.

ELM mode reserves the vehicle buses and suspends normal feature processing.
Binary USB capture keeps normal features running. See [USB diagnostics](USB_DIAGNOSTICS.md)
for lifecycle, timeouts and shared-pin behavior.

CAN TX copies each message and retains it when HAL is busy. Forwarding explicitly
converts the different RX and TX header types. UART interrupts collect messages;
command dispatch and Flash writes run in the main loop. UART and USB retain active
transmission buffers until completion. Critical sections restore the previous
interrupt state. After lost USB input, SLCAN discards the incomplete line through
the next carriage return instead of combining unrelated fragments.

`application_state.h` gathers integration state. Independent components such as
SLCAN, UDS decoding, record storage and the menu model avoid that dependency.
Host tests execute production components with hardware substitutes.

## Extending the firmware

- Add a frame handler in `vehicle`, validate its minimum DLC, declare it in
  `frame_handlers.h`, and register its CAN ID in the dispatcher.
- Add periodic behavior in `features` and call it from `powertrain_process`,
  `chassis_process` or `body_process` as appropriate.
- Follow the [menu guide](MENU_UX.md) when adding readings or actions. Keep
  existing page IDs and saved setting slots stable.
- A parameter uses `(raw + raw_offset) * scale + scaled_offset`; `raw_offset`
  is signed. UDS accepts single-frame `0x62` replies for the current ECU, DID
  and page. The menu parameter reader supports single-frame values; the separate
  BCM fault reader and ELM interpreter implement multipart replies. A multipart
  menu parameter needs explicit integration and tests, not just a catalog entry.
- Add settings through `SetupParam`, `SettingsState`, defaults and persistence.
  Changing a saved-record layout requires a new format identity and migration.

## Persistent data and compatibility

C1 now allocates 96 KiB for the program and 20 KiB for its optional disk
(`0x08018000..0x0801cfff`). Other flavors allocate 64 KiB for the program and
52 KiB for the disk (`0x08010000..0x0801cfff`). See the
[upstream integration report](UPSTREAM_SYNC.md) for measured image sizes and
new diagnostics. Records use the final 12 KiB
(`0x0801d000..0x0801ffff`). Each record has two 2048-byte pages, CRC32, generation,
type and version. The commit marker is written last. Unchanged data does not
cause an erase. Settings, performance records, menu preferences and BH mirror
positions have distinct record types.

C1 firmware, persistence and MSC require **128 KiB of physically reported Flash**.
The new C1 image cannot execute on a 64 KiB MCU. Other flavors still reject
storage access on such parts and use preferences in RAM. The initial
architecture migration changed the original firmware's disk geometry and saved
settings: preserve needed files and settings before upgrading, then restore
preferences and mirror calibration. Update the complete board set. The later menu
migration from visibility record `0x103` to preferences `0x104` is described in
the menu guide; it does not import arbitrary original firmware 3.1.1 data.

Recoverable records do not make FAT transactional: interruption during a file
sector write can still damage the file. MSC is read-only from the host. The legacy
`hello.txt` command remains a demonstration, not a CAN traffic recorder.

## Tools and review conventions

The Makefile is the supported build definition. `baccable.ioc` is a hardware
configuration reference; regenerating it does not recreate this architecture.
Use an external-Makefile project in an IDE. The root `.clang-format` defines the
application style; do not reformat vendor libraries.

Write project documentation and user-facing text in English. Keep function
comments to one or two English sentences describing their purpose.
Document protocol details and ownership constraints where they affect correctness.
Keep unused research outside executable functions, and check callbacks, build
flags and startup vectors before removing apparently unreferenced symbols.

CI runs ASan/UBSan host tests, cppcheck and ARM GCC for C1/C2/BH/CAN. Stable and beta
releases share validation before publication. DEBUG_MODE excludes the CAN variant.
