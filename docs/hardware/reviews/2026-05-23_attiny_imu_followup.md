# Schematic review follow-up 2026-05-23: ATtiny UPDI and IMU updates

## Scope

Reviewed pushed updates after:

- adding ATtiny1616 UPDI/programming VCC selection,
- adding the internal-pull-up note for the input devices,
- updating IMU handling/test points,
- marking unused XIAO breakout camera I2C lines as NC.

## ATtiny1616 / UPDI

The jumper-based VCC selection is the right direction.

Current visible intent:

```text
PERIPH_3V3 -- JP2 ATTINY_VCC -- UPDI_VCC / ATtiny VCC
J4:
  pin 1 = UPDI_VCC
  pin 2 = GND
  pin 3 = PA0_UPDI through R6 470Ω
```

This lets the ATtiny be normally powered from `PERIPH_3V3`, while allowing the ATtiny-side VCC node to be separated for programmer-powered work.

### Important check: decoupling capacitor location

In the screenshot, C8 appears to be on the `PERIPH_3V3` side of JP2. If JP2 is open and the programmer powers `UPDI_VCC`, C8 may no longer decouple the ATtiny VCC pin.

Recommendation:

- Put the ATtiny local 0.1µF decoupling capacitor on the ATtiny VCC / `UPDI_VCC` side, as close to U3 VCC/GND as practical.
- If desired, keep an additional capacitor on `PERIPH_3V3`, but the mandatory MCU decoupling should follow the MCU VCC island.

### Labeling recommendation

Add a note near JP2/J4 such as:

```text
Normal: JP2 closed, UPDI_VCC = PERIPH_3V3, programmer VCC as VREF only
Programmer-powered: JP2 open, programmer may power UPDI_VCC only
```

This prevents accidentally powering the whole `PERIPH_3V3` rail from the UPDI programmer.

### UPDI series resistor

R6 = 470Ω on PA0_UPDI is good for a test board. It adds some protection/debug friendliness without being excessive.

### Input pull-up note

The input-device note `ATtiny internal pull-up enabled` is good. The switch/encoder wiring to GND is consistent with firmware-enabled internal pull-ups.

## IMU / BNO055

The IMU unused/support pins are now brought to test points:

```text
TP13 = IMU_2V8
TP14 = GND
TP15 = IMU_INT
TP16 = IMU_RESET
```

This is useful for bring-up.

### I2C pull-ups not visible in pushed schematic

The screenshot and KiCad text currently do not show discrete I2C pull-up resistors such as:

```text
I2C_SDA -> 4.7k -> PERIPH_3V3
I2C_SCL -> 4.7k -> PERIPH_3V3
```

If the intended pull-ups are on the BNO055 module, that may be acceptable. If the intent was to add optional board-side pull-up footprints, they do not appear to be present in the pushed schematic yet.

Recommendation for test board:

- Add optional/DNP board-side pull-up footprints on SDA/SCL to `PERIPH_3V3`, or
- Add a note that the pull-ups are supplied by the BNO055 module.

### IMU_2V8 caution

Keeping `IMU_2V8` as a test point is fine. Treat it as a module-side output/reference unless the exact module documentation says otherwise. Do not connect it to `PERIPH_3V3`.

## CAM_SDA / CAM_SCL

Marking unused `CAM_SDA` and `CAM_SCL` as NC is good if they are not routed to test points or used on the test board.

## Summary

Accepted:

- VCC jumper approach for ATtiny programming,
- 470Ω UPDI series resistor,
- internal pull-up note for input devices,
- IMU support/test points,
- CAM_SDA/CAM_SCL NC marking.

Recommended before layout:

1. Move/add the ATtiny 0.1µF decoupling capacitor to the `UPDI_VCC` / ATtiny VCC side of JP2.
2. Add a clear JP2/J4 programming-power note.
3. Confirm whether I2C pull-ups are module-provided or add optional DNP 4.7k pull-ups to `PERIPH_3V3`.
