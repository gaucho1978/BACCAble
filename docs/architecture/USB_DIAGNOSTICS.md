# USB diagnostics and new menu actions

Use a matching C1/C2/BH firmware set from this integration. Verify the C1 Flash
requirement in [the integration report](UPSTREAM_SYNC.md#flash-and-ram) before
flashing. The commands below describe runtime USB, not the STM32 DFU bootloader.

## Reading pages and actions

- `Settings` → `Feature setup` → `Auto rotate`: enable to advance through the
  current list every five seconds. Use favorites to create a short rotating set.
- `Actions` → `Maximum hold`: toggle between live values and numerical maxima.
  Changing the selected page starts a new interval. An unavailable or stale signal
  displays `--` rather than an old maximum.
- `Settings` → `Feature setup` → `Allow read`: enable, then leave Feature setup. Open
  `Actions` → `Read BCM faults`; browse results with previous/next, press RES to
  retry, or go Back to leave. A `+` after the result count means the BCM returned
  more than the displayed limit of 20 codes. Reading does not clear faults.
- `PDC mute` and `Reverse mute` are independent options in Feature setup.
  They require valid current vehicle messages before acting.
- `Actions` → `IBS override` is experimental and requires action confirmation
  and a running engine. It substitutes 75% in a short burst of otherwise copied
  IBS messages when the observed SOC is within the upstream range. It is not a
  verified way to improve battery charging. It is off after startup and stops at
  engine stop or diagnostic-mode entry.

Leave Feature setup to persist committed changes automatically. New settings are initially off;
existing saved settings take priority over compiled defaults.

## ELM-compatible USB diagnostics

1. Connect the **C1 USB port** with a data-capable cable. On the dedicated board
   described in the original manual, C1 is the right-hand port, C2 the middle and
   BH the left in the manual's orientation. Prefer PCB labels; viewing the board
   from the opposite side reverses left and right.
2. Select `Settings` → `Feature setup` → `USB mode: ELM327`, then leave Feature setup. This replaces
   CAN capture. USB reconnects as a serial device; select its new port in
   the host application. Connect/open it promptly: an unconfigured USB session
   expires after ten seconds.
3. Use a terminal or diagnostic application supporting an ELM-style CAN adapter.
   Commands end with carriage return. For example, `ATI` identifies the
   interpreter, `ATZ` resets command preferences, `ATE0` disables echo and `ATH1`
   enables response addresses. A response finishes with `>`.
4. The interpreter tries the vehicle buses and remembers where each requested
   controller answered. Physical bus rates remain C1/C2 **500 kbit/s** and BH
   **125 kbit/s**. Protocol/divisor commands influence adapter preferences and
   search order; they do not reconfigure the vehicle's physical bus speeds.
5. Close the client when finished. A USB disconnect expires after ten seconds;
   120 seconds without a complete ELM command also ends the session. Auxiliary
   boards restore normal operation after their own lease expires if C1 vanishes.

While ELM diagnostics owns the buses, normal feature processing is suspended.
Do not expect the display menu to remain navigable during a diagnostic session.
The USB port shares a C1 LED-strip pin, so strip output pauses while USB uses it.

This is a bounded ELM-compatible subset, not a complete ELM327 implementation.
It supports CAN request formatting, filters, raw frames and automatic ISO-TP
assembly up to 255 response bytes, with client requests up to 32 bytes. It does
not add K-line/J1850 hardware or ECU security access. Unsupported AT commands
return `?`; unavailable replies return `NO DATA`. `ATRV` uses a cached measured
voltage if available and otherwise returns `NO DATA`. Compatibility with each
third-party diagnostic application needs testing. Remote ISO-TP flow control
requires at least 10 ms between consecutive frames to fit the inter-board link.

On macOS, look for the newly appearing `/dev/cu.usbmodem*`; on Linux, `/dev/ttyACM*`;
on Windows, use the newly enumerated COM port. Port numbers are host-assigned.

## Binary CAN capture

1. Select `USB mode: CAN` in Feature setup and leave that submenu. ELM mode is switched off.
2. Connect the USB port of the bus to record: C1 for powertrain, C2 for chassis,
   BH for body. Multiple host connections can record the buses separately.
   Eject an existing USB disk before changing its role.
3. Open the serial device as a binary stream. This mode does not produce SLCAN
   text or automatically create a CAN log on the USB disk. Normal vehicle feature
   processing continues during capture.
4. Save complete 16-byte records and decode the fields below. Each stream uses
   that board's local clock, so timestamps from different boards need alignment.
5. Disconnect to expire capture, or disable it and leave Feature setup. A connected
   auxiliary capture port keeps the board set awake. Timeout changes the current
   session; select OFF and leave Feature setup if capture should also stay off after reboot.

| Bytes | Meaning |
| --- | --- |
| 0 | `0xa0` through `0xa8`: low nibble is CAN DLC |
| 1–3 | Millisecond timestamp, little-endian, wrapping at 24 bits |
| 4–7 | CAN identifier, little-endian |
| 8–15 | CAN data, unused bytes zeroed |

A record beginning with `0xaf` reports capture loss: bytes 4–5 contain the dropped
frame count, capped at 65535; a later incoming frame emits the marker once buffer
space is available. This extension distinguishes overflow from ordinary data.
The upstream record format has no explicit standard/extended-ID flag and does
not record RTR frames. Capture is bounded and may drop data when the host cannot
keep up; the loss marker should be retained in exported logs.
