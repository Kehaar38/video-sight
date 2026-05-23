# Schematic review follow-up 2026-05-23: XIAO breakout fixes

## Scope

Reviewed the pushed updates after the first 2026-05-23 test-board review:

- separated `PERIPH_EN` from the crowded `XIAO_3V3` area,
- added the XIAO ESP32-S3 breakout-board KiCad project,
- corrected the SPI label typo around MOSI/MISO,
- confirmed the intended GPIO41/D12 pad use for `PERIPH_EN`.

## Verified fixes

### `PERIPH_EN` / `XIAO_3V3`

The updated screenshot is much clearer.

Current intended layout:

```text
XIAO_3V3 -> TP5 -> C5 -> J2 pin 1
PERIPH_EN -> TP6 -> J2 pin 3
```

This resolves the previous visual shorting concern. The two nets are presented separately now.

### GPIO41 / D12 breakout mapping

The new breakout-board schematic maps:

```text
XIAO GPIO41_D12_MIC_DATA -> J1 pin 3
```

The test board uses:

```text
J2 pin 3 -> PERIPH_EN
```

So the intended chain is:

```text
XIAO GPIO41 / D12 pad -> breakout J1 pin 3 -> cable -> test-board J2 pin 3 -> PERIPH_EN -> TPS22919 ON
```

This matches the stated design.

### SPI labels

The visible label typo is fixed. The test-board side now distinguishes:

```text
SPI_MOSI
SPI_MISO
SPI_SCK
```

## Remaining checks before PCB layout

### Connector orientation / cable pin numbering

The main remaining risk is not the schematic net names, but the physical 2x10 connector orientation between the breakout board and test board.

Before layout/order, decide and document:

- whether the interconnect is pin-header to dupont wires, IDC ribbon, JST, or another connector,
- where pin 1 is on both boards,
- whether the cable crosses/mirrors the pinout,
- whether the mating footprints are viewed from the same side or opposite sides.

For the schematic, J1 pin 3 to J2 pin 3 is correct. The PCB/cable must preserve that mapping physically.

### Power/GND conductors

The connector has multiple useful return paths:

- J2 pin 2: GND
- J2 pin 19: GND
- J2 pin 20: +BATT
- J2 pin 1: XIAO_3V3

This is acceptable, but for a cable-mounted XIAO/camera assembly, keep ground conductors near power/high-speed signals if the connector/cable choice allows it.

### Footprints still need assignment

The connector footprint fields are still empty in the KiCad schematic for the test-board/breakout connectors. This is fine at schematic stage, but should be assigned before PCB placement and ERC/BOM review.

### Pending items unchanged

The following previously noted items still need a pass:

- ATtiny1616 UPDI/VCC/GND programming access,
- input-device pull-up assumptions,
- optional I2C pull-up footprints,
- BNO055 `IMU_RESET`, `IMU_INT`, and `IMU_2V8` handling.

## Summary

The three fixes are accepted:

- `PERIPH_EN` presentation is no longer visually confused with `XIAO_3V3`,
- GPIO41/D12 pad to `PERIPH_EN` is mapped through connector pin 3,
- SPI MOSI/MISO/SCK labels are separated.

The next highest-risk item is connector/cable orientation, because it can invalidate an otherwise-correct pin-to-pin schematic mapping.
