# Unimplemented signal notes

These observations were previously stored in inactive CAN handlers. They are
reference material, not implemented features or validated decoding contracts.
Verify them against vehicle captures before adding behavior.

| CAN ID | Existing observation |
| --- | --- |
| `0x1F0` | Clutch interlock: byte 0 bit 7; upstop: bit 6. Pedal position spans byte 0 bits 4–0 and byte 1 bits 7–5; analog position spans byte 1 bits 4–0 and byte 2 bits 7–5. |
| `0x1FC` | C2 suspension/differential status. Byte 0 contains differential warning/control and damping mode; byte 1 includes damping/aero faults; byte 2 bit 5 is the CDCM warning lamp. |
| `0x2EE` | BH steering-wheel media buttons: byte 3 bits 6/4/2/0 represent right/left/voice/phone. Byte 4 is a wrapping volume counter; byte 5 bits 7–6 indicate increase/decrease/mute. |
| `0x358` | Possible volume position/direction in byte 2; the original note was unconfirmed. |

The former `0x420` battery and `0x4B4` chassis handlers had no active behavior
and no reliable additional decoding. Ordinary routing remains available through
the existing message-routing feature.

Additional removed handlers: `0xFA` (brake pedal) had no behavior, and `0x73A`
(clock) only documented a packed date/time example. The former `0x1F7` decoder
stored byte 2 bit 0 and byte 3 bits 7–3 in an unread temperature field; the
displayed transmission temperature continues to use its diagnostic parameter.
The unused coolant byte from `0x2ED` was also removed; shift guidance still uses
this frame, while the menu obtains coolant temperature through diagnostics.
