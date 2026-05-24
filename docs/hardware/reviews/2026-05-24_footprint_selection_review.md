# Footprint selection review 2026-05-24

## Scope

Reviewed pushed commit `3e11275` after footprint selection for the VIDEO SIGHT test-board schematic.

KiCad CLI is not available in this environment, so this review is based on the `.kicad_sch` text and project files rather than a full ERC/DRC run.

## Overall judgment

The footprint selection is generally appropriate for a hand-assembled first test board.

Good choices:

- 0805 hand-solder footprints for resistors and small capacitors.
- Large THT test pads for all test points.
- Pin headers for LCD, UPDI, ATtiny breakout, input device, and current/VCC jumpers.
- Keyed IDC 2x10 header for the XIAO breakout cable.
- R7/R8 are now marked DNP in KiCad, matching the schematic note.
- AO3401A uses standard SOT-23 with pin mapping already checked as `1=G, 2=S, 3=D`.

## Selected footprints observed

```text
R1-R8: 0805 hand-solder resistors
C1/C2/C5/C6/C7/C8/C9: 0805 hand-solder capacitors
C3/C4: custom electrolytic/tantalum-style footprint @自分用:CAP_FN_B_PAN
Q1: Package_TO_SOT_SMD:SOT-23
U1: custom @自分用:DCK0006A_L
U2: SOIC-20W 7.5x12.8mm P1.27mm
J1: JST-XH 2-pin vertical
J2: IDC 2x10 P2.54 vertical
J3/J5/J6/J7/J8/JP1/JP2: 2.54mm pin headers
J4: DIP-8 W7.62mm socket footprint for the BNO055 module
SW1: DIP slide switch footprint
SW2-SW5: 6mm THT push buttons
SW6: Alps EC12E vertical encoder
TP1-TP16: THT pad D1.5mm Drill0.7mm
```

## Things that look good

### DNP handling

R7/R8 are now `dnp=yes` in the schematic file. This matches the intended policy:

```text
R7/R8 optional I2C pull-ups
Default: DNP
Populate only if module pull-ups are insufficient
```

### Test points

The selected footprint:

```text
TestPoint:TestPoint_THTPad_D1.5mm_Drill0.7mm
```

is a good default for this test board. It is easy to probe and can accept temporary wires.

### Passive size

0805 hand-solder footprints are a good choice for manual assembly and rework. They are larger than necessary electrically, but well suited for a test board.

### ATtiny package

The SOIC-20W footprint is a friendly choice for hand soldering and recovery compared with smaller packages.

## Items to verify before PCB/order

### 1. Custom footprints must be committed or replaced

The schematic references custom footprint libraries:

```text
@自分用:CAP_FN_B_PAN
@自分用:DCK0006A_L
```

No project `fp-lib-table` or `.pretty` directory was found in the repository. If those footprints exist only in the local KiCad user library, another machine or CI will not be able to resolve them.

Recommended options:

1. Commit the custom `.pretty` library and project `fp-lib-table`, or
2. Replace with standard KiCad footprints if suitable.

This is especially important for `U1` / TPS22919, because the exact DCK/SOT-563-style pad geometry matters.

### 2. TPS22919 footprint/pin mapping

`U1` uses:

```text
@自分用:DCK0006A_L
```

Before layout/order, verify against the TPS22919 datasheet:

- pin 1 location,
- pad numbering,
- package variant DCK,
- thermal/assembly recommendations,
- KiCad symbol pin numbers.

### 3. C3/C4 footprint and polarity

C3/C4 are both `47uF` and use the custom `CAP_FN_B_PAN` footprint.

Check:

- capacitor type: electrolytic, tantalum, polymer, or large MLCC,
- polarity marking if polarized,
- voltage rating,
- physical height/diameter,
- pad spacing and courtyard.

If using polarized capacitors, make polarity very clear on silkscreen.

### 4. BNO055 module socket footprint

`J4` uses:

```text
Package_DIP:DIP-8_W7.62mm_Socket
```

This can work if the module header really matches 2x4, 2.54mm pitch, 7.62mm row spacing. Before PCB order, measure/check the AE-BNO055-BO board header spacing.

A generic `PinSocket_2x04_P2.54mm` footprint may be clearer than a DIP IC socket footprint if the physical part is a module header/socket.

### 5. IDC connector orientation

`J2` uses:

```text
Connector_IDC:IDC-Header_2x10_P2.54mm_Vertical
```

This matches the keyed-cable plan. Still verify in PCB:

- pin 1 mark,
- key notch direction,
- cable exits in the intended direction,
- straight-through cable maps J1 pin N to J2 pin N,
- no mirrored orientation between breakout and test board.

### 6. LCD header mechanical fit

`J3` is a normal 1x08 vertical pin header. Confirm this matches the Waveshare module orientation and whether the module should plug directly, use wires, or use a socket.

### 7. Switch and encoder footprints

The selected footprints are plausible, but they depend heavily on exact parts:

- `SW_PUSH_6mm` for tactile switches,
- `RotaryEncoder_Alps_EC12E_Vertical_H20mm` for encoder,
- DIP slide switch footprint for SW1.

Before ordering, compare to the actual part datasheets or the parts in hand.

## Summary

Accepted as a test-board footprint baseline:

- 0805 hand-solder passives,
- THT test points,
- pin headers/jumpers,
- SOIC-20W ATtiny,
- SOT-23 AO3401A,
- DNP status for R7/R8.

Main blocker before a reproducible PCB project:

- custom footprints `@自分用:*` need to be committed into the repo or replaced with standard/library footprints.

Main mechanical checks before PCB order:

- TPS22919 DCK footprint pin mapping,
- BNO055 module row spacing,
- IDC key orientation,
- LCD header orientation,
- switch/encoder exact part footprints.
