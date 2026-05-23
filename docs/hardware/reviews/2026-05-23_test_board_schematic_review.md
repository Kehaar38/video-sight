# Schematic review 2026-05-23: test-board update

## Scope

Reviewed the updated KiCad schematic and screenshots after:

- adding test points,
- replacing directly connected modules with headers/connectors,
- adding the XIAO breakout-board connector,
- exposing GPIO39-GPIO42 pads,
- moving `PERIPH_EN` from D6/GPIO43 to GPIO41/D12 pad,
- returning D6/GPIO43 to `LCD_BL`,
- making ATtiny1616 I/O available through jumpers.

## Good changes

- The test-point coverage is much better: battery input, switched battery, XIAO BAT, `XIAO_3V3`, `PERIPH_3V3`, `PERIPH_EN`, VBUS, SPI, and GND are visible.
- Header-based modules make sense for the large test board and temporary enclosure work.
- D6/GPIO43 returning to `LCD_BL` is useful; backlight PWM may become valuable during current/brightness testing.
- Using GPIO41/D12 pad for `PERIPH_EN` is a good direction if the XIAO breakout board makes the back-side pads easy to reach.
- ATtiny1616 I/O headers are appropriate for a test board; they keep button/encoder routing flexible.
- TPS22919 block still looks structurally correct: `XIAO_3V3` -> TPS22919 -> `PERIPH_3V3`, EN pulldown, QOD resistor, input/output capacitors.
- Battery block still looks structurally correct: battery -> current jumper -> switch -> AO3401A -> `+BATT`, with divider after protection.

## Issues / checks before layout

### 1. `PERIPH_EN` appears to be on the same wire as `XIAO_3V3` in the XIAO breakout screenshot

In the XIAO screenshot, the wire from the `XIAO_3V3` power symbol to J2 pin 1 also has the `PERIPH_EN` label and TP6 attached. If this is how the schematic is actually connected, `PERIPH_EN` is shorted to `XIAO_3V3`, which would keep TPS22919 always enabled and also drive the EN net from the 3.3V rail.

Expected:

- J2 pin for XIAO 3V3 should be only `XIAO_3V3`.
- `PERIPH_EN` should connect only to the intended GPIO41/D12 pad line plus TPS22919 ON and TP6.
- Do not put a power symbol and a signal label on the same wire unless the short is intentional.

### 2. Verify GPIO41 actually reaches `PERIPH_EN`

The stated intent is `GPIO41/D12 pad = PERIPH_EN`, but the screenshot does not clearly show a `GPIO41` net. Confirm in KiCad by highlighting `PERIPH_EN` and checking it lands on the GPIO41/D12 breakout pin, not on `XIAO_3V3` or GPIO42.

### 3. Check SPI labels on the XIAO breakout connector

The screenshot visually suggests two right-side pins may both be labeled `SPI_MOSI`. Since the current pinout expects SPI SCK/MISO/MOSI, verify the connector has the intended mapping:

- SPI_SCK
- SPI_MISO, if the test board needs it
- SPI_MOSI

The LCD needs SCK and MOSI; MISO may be unused by the LCD, but duplicated MOSI labels are a common source of layout mistakes.

### 4. ATtiny1616 needs an explicit UPDI programming path

ATtiny1616 uses PA0/RESET as UPDI. The PA0 pin is exposed on the right-side header, which can work, but add or clearly label a 3-pin programming header/test pads:

```text
UPDI / VCC / GND
```

This will make recovery and programming much easier on the test board.

### 5. ATtiny input devices rely on pull-ups

The buttons and rotary encoder common appear tied to GND. That is fine if firmware enables internal pull-ups on the ATtiny pins. Add a schematic note or optional pull-up footprints if uncertain.

### 6. I2C pull-ups are not obvious

BNO055 modules often include pull-ups, but the bus also includes ATtiny1616. Decide whether the test board should include optional I2C pull-up footprints, probably DNP by default:

```text
SDA -> 4.7k -> PERIPH_3V3
SCL -> 4.7k -> PERIPH_3V3
```

### 7. IMU_RESET / IMU_INT / IMU_2V8 should be intentional

The BNO055 header exposes `IMU_RESET`, `IMU_INT`, and `IMU_2V8`. If these are not used, mark them NC or route them to optional pads/jumpers. Do not accidentally drive a module's 2.8V output rail.

### 8. Connector footprints still need assignment

Several headers/connectors appear to have empty footprint fields. That is fine during schematic drafting, but assign footprints before PCB layout and before ERC/BOM review.

## Summary

The architecture is moving in a good direction for a test board. The main thing to check immediately is the `PERIPH_EN` net around the XIAO breakout connector: it should not be tied to `XIAO_3V3`, and it should clearly land on GPIO41/D12. After that, the next practical checks are UPDI access, optional I2C pull-ups, and connector footprints.
