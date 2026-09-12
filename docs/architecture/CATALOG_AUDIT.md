# Reading catalog audit and engine-aware menu

All **64 gasoline and 60 diesel pages** retain their IDs, physical order,
parameter IDs, templates and scales. Classification is a browsing policy, not
proof that every ECU supports every inherited diagnostic request.

## Profile and compatibility

`Settings > Feature setup > Engine profile` cycles **2.0 I4**, **2.9 V6**, **2.2 D**.
The legacy fuel selector stays in settings slot 16; slot 36 adds gasoline V6
capability, and slot 37 adds Advanced pages. Unused zero-filled, erased or invalid
new slots default to zero. Existing diesel remains diesel; existing gasoline
defaults to I4, so V6 owners must select V6 once. The 40-slot settings record and
80-byte menu preferences are unchanged.

Engine filtering applies to readings, editors, Favorites and automatic page entry.
Incompatible Favorites remain saved and return with the corresponding profile.
I4/V6 share the existing gasoline Favorites and manual visibility preferences.
Temporarily hidden Favorites still occupy their saved slots; switch profiles to
remove them if needed. Reordering skips incompatible entries without deleting them.
Advanced pages are hidden from ordinary browsing by default. Their switch reveals
them subject to manual visibility. Editors and explicit Favorites bypass the
advanced filter, but never engine eligibility. Old firmware may clear the new
slots when saving; gasoline/diesel remains backward compatible.

## Menu roles

`Functions` becomes **Actions**. Setup switches say `Allow ...`: they permit an
action; Actions executes it. Master-enable fields, confirmations and commands are
unchanged. The old Immobilizer action only displayed a generic status message.
Its actual `+`/`-` state moves to the fifth Information page. Its separate existing
control gesture and persisted setting remain unchanged.

## Duplicates and redundancy

No two entries within either fuel catalog have both the same parameter tuple and
rendering template. Shared layouts across fuel catalogs are intentional. Repeated
parameter IDs within a single-reading page are legacy two-slot storage, not
separate duplicate menu items.

- **Oil:** volume, height, quality and pressure have different meanings and units.
  Compound layouts overlap with single pages; secondary arrangements are advanced.
- **Power:** combined Power / torque and the single pages overlap but offer
  different precision/space. All three remain ordinary.
- **Battery:** voltage, current and SOC are distinct. Native IBS SOC (parameter 3),
  ECM SOC (21, ECU 10 / DID 19BD), and BCM SOC (34, ECU 40 / DID 1005) are different
  sources. Page 0x16 is clarified as `Batt charge BCM`. Raw/source-comparison views
  are advanced; they are not interchangeable with normal battery readings.
- **Misfires:** the four-cylinder overview remains ordinary; individual cylinder
  pages are advanced. There are no verified cylinder 5/6 misfire definitions to
  add. V6 therefore retains partial coverage, explicitly labelled cylinders 1–4.
- **Temperatures/DPF:** dedicated readings remain useful; most secondary compound
  layouts and service statistics are advanced.

## Engine-specific and suspicious data

- Ignition 5/6 (0x2f/0x30) requires V6. MultiAir temperature (0x14) and the mixed
  MultiAir page (0x40) require I4. See the [manufacturer engine description](https://www.media.stellantis.com/uk-en/alfa-romeo/press/the-alfa-romeo-giulia).
- Diesel pages never enter gasoline lists. AdBlue pages also require SCR equipment;
  the fuel/profile selector cannot establish its presence. Hide these manually on
  earlier non-SCR vehicles. ECU support still needs physical validation.
- **`element_count = 0` means exactly two elements**, not autodetection. Battery
  sources (0x3b/0xb8), Battery IBS raw (0x3c/0xb9), and Oil volume/mm (0x3f) each
  contain two parameter IDs and are valid. Explicit three/four-element pages stay
  unchanged.
- `DPF load load` (0x8c) was a repeated word, corrected to `DPF load`.
- Key ID uses a numeric field and should not be interpreted as a complete
  hexadecimal identifier. IBS raw values are uninterpreted bytes. Keep both
  advanced. No speculative changes to decoding, offsets or scales were made.

## Validation

Host tests cover engine eligibility, stable ID/index mapping, Favorites round-trip
and temporary filtering, advanced/editor visibility, legacy slot defaults,
profile cycling/persistence values, action gates and the relocated Immobilizer
status. Catalog width tests run for both display lengths. Filtering does not add
heap allocation or change catalog capacity.

## All pages

Default means ordinary browsing if manually visible. Advanced requires its switch,
an editor or an explicit Favorite. Overlap describes useful alternative layouts
or sources, not identical duplicates.

| ID | Page | Profile | Visibility | Assessment |
|---|---|---|---|---|
| `0x01` | Power / torque | I4 / V6 | Default | Overlap: combined/source view |
| `0x02` | Oil bar/coolant | I4 / V6 | Advanced | Overlap: combined/source view |
| `0x03` | Oil bar / temp | I4 / V6 | Advanced | Overlap: combined/source view |
| `0x04` | Oil/coolant temp | I4 / V6 | Default | Overlap: combined/source view |
| `0x05` | Oil level/qual. | I4 / V6 | Advanced | Overlap: combined/source view |
| `0x06` | Batt charge / A | I4 / V6 | Advanced | Overlap: combined/source view |
| `0x07` | Battery V / A | I4 / V6 | Default | Overlap: combined/source view |
| `0x08` | Power | I4 / V6 | Default | Overlap: dedicated reading |
| `0x09` | Torque | I4 / V6 | Default | Overlap: dedicated reading |
| `0x0a` | Intercooler out | I4 / V6 | Default | Overlap: dedicated reading |
| `0x0b` | Intercooler in | I4 / V6 | Default | Overlap: dedicated reading |
| `0x0c` | Intake abs press | I4 / V6 | Advanced | Measurement/status |
| `0x0d` | Boost pressure | I4 / V6 | Default | Measurement/status |
| `0x0e` | Turbo sensor V | I4 / V6 | Advanced | Technical; raw/interpretation limits |
| `0x0f` | Distance (ECU) | I4 / V6 | Advanced | Measurement/status |
| `0x10` | Oil volume | I4 / V6 | Default | Overlap: dedicated reading |
| `0x11` | Oil pressure | I4 / V6 | Default | Measurement/status |
| `0x12` | Oil temp (ECU) | I4 / V6 | Default | Overlap: dedicated reading |
| `0x13` | Oil quality | I4 / V6 | Advanced | Overlap: dedicated reading |
| `0x14` | MultiAir temp | I4 only | Default | Overlap: dedicated reading |
| `0x15` | Gearbox temp | I4 / V6 | Default | Overlap: dedicated reading |
| `0x16` | Batt charge BCM | I4 / V6 | Default | Overlap: dedicated reading |
| `0x17` | Battery current | I4 / V6 | Default | Overlap: dedicated reading |
| `0x18` | Battery voltage | I4 / V6 | Default | Overlap: dedicated reading |
| `0x19` | A/C pressure | I4 / V6 | Advanced | Measurement/status |
| `0x1a` | Gear | I4 / V6 | Default | Measurement/status |
| `0x1b` | Engine run time | I4 / V6 | Advanced | Measurement/status |
| `0x1c` | Over-rev time | I4 / V6 | Advanced | Measurement/status |
| `0x1d` | Over-rev count | I4 / V6 | Advanced | Measurement/status |
| `0x1e` | Exhaust temp | I4 / V6 | Default | Measurement/status |
| `0x1f` | Catalyst temp | I4 / V6 | Default | Measurement/status |
| `0x20` | Coolant temp | I4 / V6 | Default | Overlap: dedicated reading |
| `0x21` | Knock sensor | I4 / V6 | Advanced | Technical; raw/interpretation limits |
| `0x22` | Key ID | I4 / V6 | Advanced | Technical; raw/interpretation limits |
| `0x23` | Ignition cyl 1 | I4 / V6 | Default | Measurement/status |
| `0x24` | Ignition cyl 2 | I4 / V6 | Default | Measurement/status |
| `0x25` | Ignition cyl 3 | I4 / V6 | Default | Measurement/status |
| `0x26` | Ignition cyl 4 | I4 / V6 | Default | Measurement/status |
| `0x27` | DNA mode | I4 / V6 | Default | Measurement/status |
| `0x28` | Speed | I4 / V6 | Default | Measurement/status |
| `0x29` | Seatbelt alarm | I4 / V6 | Advanced | Measurement/status |
| `0x2a` | 0-100 km/h | I4 / V6 | Default | Measurement/status |
| `0x2b` | 100-200 km/h | I4 / V6 | Default | Measurement/status |
| `0x2c` | Best 0-100 | I4 / V6 | Default | Measurement/status |
| `0x2d` | Best 100-200 | I4 / V6 | Default | Measurement/status |
| `0x2e` | Pedal map | I4 / V6 | Default | Measurement/status |
| `0x2f` | Ignition cyl 5 | V6 only | Default | Measurement/status |
| `0x30` | Ignition cyl 6 | V6 only | Default | Measurement/status |
| `0x31` | Oil height | I4 / V6 | Advanced | Overlap: dedicated reading |
| `0x32` | Misfires total | I4 / V6 | Default | Overlap: dedicated reading |
| `0x33` | Misfires cyl 1 | I4 / V6 | Advanced | Overlap: dedicated reading; V6 only cylinders 1–4 |
| `0x34` | Misfires cyl 2 | I4 / V6 | Advanced | Overlap: dedicated reading; V6 only cylinders 1–4 |
| `0x35` | Misfires cyl 3 | I4 / V6 | Advanced | Overlap: dedicated reading; V6 only cylinders 1–4 |
| `0x36` | Misfires cyl 4 | I4 / V6 | Advanced | Overlap: dedicated reading; V6 only cylinders 1–4 |
| `0x37` | Misfires cyl 1-4 | I4 / V6 | Default | Overlap: combined/source view; V6 only cylinders 1–4 |
| `0x38` | Oil L / mm / % | I4 / V6 | Advanced | Overlap: combined/source view |
| `0x39` | Oil/water/IC C | I4 / V6 | Advanced | Overlap: combined/source view |
| `0x3a` | Oil/water/gear C | I4 / V6 | Advanced | Overlap: combined/source view |
| `0x3b` | Battery sources | I4 / V6 | Advanced | Overlap: combined/source view |
| `0x3c` | Battery IBS raw | I4 / V6 | Advanced | Technical; raw/interpretation limits |
| `0x3d` | IBS raw0/SOC/V | I4 / V6 | Advanced | Technical; raw/interpretation limits |
| `0x3e` | IBS raw1/SOC/V | I4 / V6 | Advanced | Technical; raw/interpretation limits |
| `0x3f` | Oil volume/mm | I4 / V6 | Advanced | Overlap: combined/source view |
| `0x40` | Misfires/MA temp | I4 only | Advanced | Overlap: combined/source view |
| `0x81` | Power / torque | Diesel | Default | Overlap: combined/source view |
| `0x82` | Oil bar/coolant | Diesel | Advanced | Overlap: combined/source view |
| `0x83` | Oil bar / temp | Diesel | Advanced | Overlap: combined/source view |
| `0x84` | Oil/coolant temp | Diesel | Default | Overlap: combined/source view |
| `0x85` | Oil level/qual. | Diesel | Advanced | Overlap: combined/source view |
| `0x86` | Batt charge / A | Diesel | Advanced | Overlap: combined/source view |
| `0x87` | Battery V / A | Diesel | Default | Overlap: combined/source view |
| `0x88` | DPF load / temp | Diesel | Default | Overlap: combined/source view |
| `0x89` | DPF regen / temp | Diesel | Advanced | Overlap: combined/source view |
| `0x8a` | Power | Diesel | Default | Overlap: dedicated reading |
| `0x8b` | Torque | Diesel | Default | Overlap: dedicated reading |
| `0x8c` | DPF load | Diesel | Default | Overlap: dedicated reading |
| `0x8d` | DPF temp | Diesel | Default | Overlap: dedicated reading |
| `0x8e` | DPF regen % | Diesel | Default | Overlap: dedicated reading |
| `0x8f` | DPF regen mode | Diesel | Default | Measurement/status |
| `0x90` | Since DPF regen | Diesel | Default | Measurement/status |
| `0x91` | DPF regen count | Diesel | Advanced | Measurement/status |
| `0x92` | DPF avg interval | Diesel | Advanced | Measurement/status |
| `0x93` | DPF avg duration | Diesel | Advanced | Measurement/status |
| `0x94` | Battery voltage | Diesel | Default | Overlap: dedicated reading |
| `0x95` | Battery charge | Diesel | Default | Overlap: dedicated reading |
| `0x96` | Battery current | Diesel | Default | Overlap: dedicated reading |
| `0x97` | Oil quality | Diesel | Advanced | Overlap: dedicated reading |
| `0x98` | Oil temp | Diesel | Default | Overlap: dedicated reading |
| `0x99` | Oil pressure | Diesel | Default | Overlap: dedicated reading |
| `0x9a` | Oil level | Diesel | Default | Overlap: dedicated reading |
| `0x9b` | AdBlue volume | Diesel with SCR | Default | Measurement/status |
| `0x9c` | AdBlue level | Diesel with SCR | Default | Measurement/status |
| `0x9d` | Gearbox temp | Diesel | Default | Overlap: dedicated reading |
| `0x9e` | Exhaust temp | Diesel | Default | Measurement/status |
| `0x9f` | Gear | Diesel | Default | Measurement/status |
| `0xa0` | Coolant temp | Diesel | Default | Overlap: dedicated reading |
| `0xa1` | EGR target | Diesel | Advanced | Measurement/status |
| `0xa2` | EGR actual | Diesel | Advanced | Measurement/status |
| `0xa3` | Turbo target bar | Diesel | Advanced | Measurement/status |
| `0xa4` | Turbo target % | Diesel | Advanced | Measurement/status |
| `0xa5` | Turbo temp | Diesel | Default | Measurement/status |
| `0xa6` | Turbo actual bar | Diesel | Default | Measurement/status |
| `0xa7` | Turbo actual % | Diesel | Advanced | Measurement/status |
| `0xa8` | Boost target | Diesel | Default | Measurement/status |
| `0xa9` | Intake sensor V | Diesel | Advanced | Technical; raw/interpretation limits |
| `0xaa` | Fuel pressure | Diesel | Default | Measurement/status |
| `0xab` | Fuel temp | Diesel | Default | Measurement/status |
| `0xac` | Distance (ECU) | Diesel | Advanced | Measurement/status |
| `0xad` | A/C pressure | Diesel | Advanced | Measurement/status |
| `0xae` | Fuel rate | Diesel | Default | Measurement/status |
| `0xaf` | Intake air temp | Diesel | Default | Measurement/status |
| `0xb0` | Speed | Diesel | Default | Measurement/status |
| `0xb1` | Seatbelt alarm | Diesel | Advanced | Measurement/status |
| `0xb2` | 0-100 km/h | Diesel | Default | Measurement/status |
| `0xb3` | 100-200 km/h | Diesel | Default | Measurement/status |
| `0xb4` | Best 0-100 | Diesel | Default | Measurement/status |
| `0xb5` | Best 100-200 | Diesel | Default | Measurement/status |
| `0xb6` | DNA mode | Diesel | Default | Measurement/status |
| `0xb7` | Pedal map | Diesel | Default | Measurement/status |
| `0xb8` | Battery sources | Diesel | Advanced | Overlap: combined/source view |
| `0xb9` | Battery IBS raw | Diesel | Advanced | Technical; raw/interpretation limits |
| `0xba` | IBS raw0/SOC/V | Diesel | Advanced | Technical; raw/interpretation limits |
| `0xbb` | IBS raw1/SOC/V | Diesel | Advanced | Technical; raw/interpretation limits |
| `0xbc` | Oil/water/gear C | Diesel | Advanced | Overlap: combined/source view |
