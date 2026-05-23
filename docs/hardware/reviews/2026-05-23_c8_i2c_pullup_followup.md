# Schematic review follow-up 2026-05-23: C8 and I2C pull-ups

## Scope

Reviewed pushed update `367950b`:

- moved C8 around the ATtiny1616 VCC jumper,
- added board-side I2C pull-up resistors R7/R8,
- checked the AE-BNO055-BO module documentation image.

## ATtiny1616 C8

C8 is now on the `UPDI_VCC` / ATtiny VCC side of JP2:

```text
PERIPH_3V3 -- JP2 -- UPDI_VCC / ATtiny VCC
                         |
                        C8 0.1uF
                         |
                        GND
```

This is the correct side for the MCU local decoupling capacitor. It will still decouple the ATtiny when JP2 is open and the programmer powers `UPDI_VCC` directly.

## Board-side I2C pull-ups

The added topology is correct:

```text
PERIPH_3V3
  ├─ R7 4.7k -> I2C_SDA
  └─ R8 4.7k -> I2C_SCL
```

Pulling SDA/SCL to `PERIPH_3V3` is the right rail because BNO055 and ATtiny1616 are on the switched peripheral 3.3V domain.

## AE-BNO055-BO module pull-ups

The provided module documentation appears to show pull-up resistors already present on the module:

- parts list: `R1-R5 = RK73B1ETTP103J`, where `103` means 10kΩ,
- schematic: R1/R2 appear associated with the I2C signal lines and the module's 2.8V domain.

So the module may already provide I2C pull-ups, likely around 10kΩ. The board-side 4.7k pull-ups are therefore best treated as optional/DNP until measured or confirmed.

## Electrical implication if both are populated

If the module has 10k pull-ups and the board also installs 4.7k pull-ups, the effective pull-up becomes stronger:

```text
4.7k || 10k ≈ 3.2kΩ
```

That is usually acceptable for a short 3.3V I2C bus.

If the module pull-ups are to 2.8V while the board pull-ups are to 3.3V, the idle level becomes a weighted value, roughly around 3.1V with 10k-to-2.8V and 4.7k-to-3.3V. This is probably still recognized as High by 3.3V devices, but mixing pull-up rails is not as clean as using one defined pull-up rail.

## Recommendation

For the test board:

- Keep R7/R8 footprints.
- Mark R7/R8 as DNP by default, or choose footprints that are easy to leave open/populate later.
- During bring-up, first measure SDA/SCL idle voltage with the BNO055 module installed and R7/R8 unpopulated.
- If idle level/rise time is weak, populate R7/R8.

Suggested schematic note:

```text
R7/R8: optional I2C pull-ups to PERIPH_3V3; DNP if BNO055 module pull-ups are sufficient.
```

## Summary

Accepted:

- C8 placement is now correct.
- R7/R8 topology is correct for board-side optional I2C pull-ups.

Recommended change before layout/BOM:

- Make R7/R8 optional/DNP by default or add a note, because the AE-BNO055-BO module likely already includes 10k I2C pull-ups.
