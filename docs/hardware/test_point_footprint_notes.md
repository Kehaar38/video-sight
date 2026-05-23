# Test point footprint notes

## Goal

Choose practical test-point footprints for the first VIDEO SIGHT test board.

The board has enough space, so prioritize ease of hand probing, bring-up, and rework over density.

## Recommended default

Use through-hole loop / turret-style or large plated test pads for most important test points.

Recommended KiCad-style choices:

```text
Important power/debug rails:
  TestPoint:TestPoint_THTPad_D1.5mm_Drill0.7mm
  or similar 1.5mm+ plated pad

Dense or low-priority signal-only points:
  TestPoint:TestPoint_Pad_D1.0mm
  or similar 1.0mm SMD pad
```

If a wire may be soldered during bring-up, prefer THT pads over tiny SMD pads.

## By signal class

### Power rails and grounds

Use larger THT pads or loop test points.

Signals:

```text
BAT+
BAT_SW
XIAO_BAT
XIAO_3V3
PERIPH_3V3
VBUS
GND
```

Recommendation:

```text
1.5mm to 2.0mm plated pad, 0.7mm to 1.0mm drill
```

Rationale:

- easy multimeter probing,
- can clip or solder temporary wires,
- useful for current/voltage bring-up,
- mechanically stronger.

Place at least two or three GND test points around the board if space allows.

### Control and reset lines

Use medium pads; THT is still preferred on a test board.

Signals:

```text
PERIPH_EN
LCD_BL
LCD_RST
IMU_RESET
WAKE
PA0_UPDI
```

Recommendation:

```text
1.0mm to 1.5mm pad
THT if likely to be jumpered; SMD pad if only oscilloscope probing
```

### Digital buses

Use small-to-medium SMD pads or compact THT pads.

Signals:

```text
SPI_MOSI
SPI_MISO
SPI_SCK
LCD_CS
LCD_DC
I2C_SDA
I2C_SCL
IMU_INT
```

Recommendation:

```text
1.0mm SMD pad, or 1.0mm/0.5mm THT pad if temporary wires may be soldered
```

For oscilloscope/logic analyzer probing, place a nearby GND point.

### Observe-only module rails

Signals:

```text
IMU_2V8
```

Recommendation:

```text
small or medium pad, clearly labeled observe-only
```

Do not make it look like a normal power input/output rail.

## Preferred scheme for this board

Because this is a spacious first test board, use two sizes:

```text
TP_BIG:
  THT pad, D1.5mm to D2.0mm, drill 0.7mm to 1.0mm
  For power rails, GND, current/bring-up points, UPDI/power-control lines.

TP_SMALL:
  SMD pad D1.0mm or compact THT pad
  For SPI/I2C/control observations.
```

If keeping things simpler, use the larger THT pad for almost all test points. The board is for bring-up, not production density.

## Layout notes

- Put GND test points near SPI/I2C groups for oscilloscope ground reference.
- Put one GND near battery/power input and one near peripheral rail.
- Label each TP with both reference and net name if space allows.
- Keep high-current paths short and wide; test pads should not accidentally become narrow series bottlenecks.
- For power rails, avoid tiny SMD-only points unless the signal is only for voltage measurement.
- For optional jumper-like debugging, use 2-pin headers or solder jumpers, not ordinary one-pad test points.

## VIDEO SIGHT recommendation

For the first test board:

```text
Power/GND/UPDI/PERIPH_EN:
  large THT test pads, about 1.5mm pad / 0.7mm drill or larger

SPI/I2C/LCD/IMU_INT:
  1.0mm SMD pads are acceptable, but THT pads are more convenient if space allows

GND:
  use several large THT pads across the board
```

This makes the board easier to debug with a multimeter, scope probe, logic analyzer, or temporary jumper wires.
