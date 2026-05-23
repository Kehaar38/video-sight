# Final schematic review 2026-05-23 test-board draft

## Scope

Reviewed the final 2026-05-23 pushed schematic screenshots:

- battery / reverse-protection / ADC divider,
- XIAO breakout connector and I2C pull-up option,
- TPS22919 load switch,
- 1.89 inch LCD header,
- BNO055 IMU module header,
- ATtiny1616 sub-MCU and input-device wiring.

## Overall judgment

The schematic is in good shape for a first test-board draft.

The major earlier risks have been addressed:

- `PERIPH_EN` is separated from `XIAO_3V3`.
- GPIO41/D12 is routed through the breakout connector to `PERIPH_EN`.
- D6/GPIO43 is back to `LCD_BL`.
- SPI MOSI/MISO/SCK labels are distinct.
- ATtiny UPDI programming has a VCC isolation jumper.
- ATtiny local C8 decoupling is on the `UPDI_VCC` / ATtiny VCC side.
- I2C board-side pull-ups are present and marked optional/DNP in the schematic note.

## Block-by-block notes

### Battery / reverse protection

The battery block is clear:

```text
BATTERY -> BAT_CURRENT jumper -> SW_BATT -> AO3401A -> +BATT / XIAO_BAT
```

The battery ADC divider also looks appropriate:

```text
+BATT -> 470k -> BATT_ADC -> 220k -> GND
BATT_ADC -> 0.047uF -> GND
```

Keep the existing requirement: before PCB order, verify AO3401A symbol pin numbering against the chosen footprint and purchased part.

### XIAO breakout / I2C pull-ups

The XIAO breakout connector is readable and the pin mapping is now clear.

R7/R8 topology is correct:

```text
PERIPH_3V3 -> 4.7k -> I2C_SDA
PERIPH_3V3 -> 4.7k -> I2C_SCL
```

The text note says:

```text
R7/R8: optional I2C pull-ups to PERIPH_3V3
Default: DNP
Populate if module pull-ups are insufficient
```

This is the right policy. One KiCad/BOM detail: R7/R8 currently appear as normal populated parts (`dnp no`) in the schematic file. Before generating BOM/placement files, either set their KiCad DNP attribute or keep a clear manual BOM note.

### Load switch

The TPS22919 block is good:

```text
XIAO_3V3 -> TPS22919 IN
PERIPH_EN -> ON with 100k pulldown
TPS22919 OUT -> PERIPH_3V3
QOD -> 1k -> OUT
PERIPH_3V3 bulk/decoupling: 1uF + 47uF
```

This matches the intended low-power peripheral rail.

Before PCB order, verify the TPS22919 DCK footprint (`DCK0006A_N`) with the exact package/datasheet pinout.

### LCD

LCD header is straightforward:

```text
PERIPH_3V3, GND, SPI_MOSI, SPI_SCK, LCD_CS, LCD_DC, LCD_RST, LCD_BL
```

This matches the current allocation.

Note: if LCD_BL is a raw LED input rather than a logic/PWM enable input on the Waveshare module, confirm whether direct GPIO drive is acceptable or whether a transistor is needed. If the module already has a backlight driver/transistor, the current connection is fine.

### IMU / BNO055

The IMU header and test points look good:

```text
PERIPH_3V3, GND, I2C_SDA, I2C_SCL
IMU_2V8, IMU_INT, IMU_RESET test points
```

Keep `IMU_2V8` as observe-only unless the module documentation explicitly says otherwise.

### ATtiny1616 / inputs

The ATtiny block is good for a flexible test board:

- JP2 isolates ATtiny VCC / `UPDI_VCC` from `PERIPH_3V3`.
- C8 is on the MCU side of JP2.
- J4 exposes `UPDI_VCC`, GND, and PA0/UPDI through 470Ω.
- J5/J6 expose spare ATtiny pins for jumper experiments.
- J7 input device and switches use GND-return wiring with an internal pull-up note.

Recommended silkscreen/note near JP2/J4:

```text
Normal: JP2 closed, UPDI VCC is VREF only
Programmer power: JP2 open, power UPDI_VCC only
```

## Remaining pre-layout checks

Before moving to PCB/order, the main remaining work is mechanical/implementation detail rather than circuit concept:

1. Assign final footprints for all headers, test points, passives, switches, and connectors.
2. Set R7/R8 to KiCad DNP or explicitly exclude them in the first BOM.
3. Verify AO3401A and TPS22919 symbol/footprint pin mapping against datasheets.
4. Select keyed IDC connector footprints and confirm pin-1 orientation on both XIAO breakout and test board.
5. Decide LCD_BL electrical drive after checking the Waveshare LCD module backlight input.
6. Run KiCad ERC and review all NC markers/no-connect pins.
7. Add PCB silkscreen labels for power rails, jumper normal positions, UPDI warning, and connector pin 1.

## Summary

This is now a reasonable schematic baseline for the first VIDEO SIGHT test board.

No major schematic-level blocker is visible in the screenshots. The remaining risks are mostly:

- footprint/pinout mismatch,
- connector orientation,
- DNP/BOM handling for optional I2C pull-ups,
- LCD backlight drive confirmation.
