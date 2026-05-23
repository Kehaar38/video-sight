# ATtiny1616 UPDI programming power notes

## Question

A simple 3-pin UPDI header was considered:

```text
1: PERIPH_3V3
2: GND
3: PA0 / UPDI
```

Concern: if a programmer supplies VCC on pin 1, the programmer will energize the entire `PERIPH_3V3` rail, including other peripherals and the TPS22919 OUT side.

## Judgment

That concern is valid.

Do not treat the UPDI header VCC pin as an unconditional external power input for the ATtiny1616 while it is directly tied to `PERIPH_3V3`.

If the programmer drives VCC into this pin, it can power:

- ATtiny1616,
- BNO055 module,
- LCD VCC side,
- any pull-ups/loads on `PERIPH_3V3`,
- TPS22919 VOUT side.

This may also create unwanted reverse/back-power conditions toward the TPS22919 input side depending on the load-switch behavior and surrounding circuitry.

## Recommended approaches

### Preferred: target-powered UPDI header

Use the header VCC pin as target voltage reference/sense only, not as programmer-supplied power.

```text
UPDI header:
  VREF = PERIPH_3V3
  GND
  UPDI = PA0
```

Programming procedure:

1. Power the board normally.
2. Ensure `PERIPH_3V3` is ON.
3. Programmer uses VREF to set 3.3V signal level.
4. Programmer does not source power into VREF.

This is cleanest for in-circuit programming.

### Add a manual PERIPH_EN force option

Because ATtiny1616 is behind TPS22919, it is not powered unless `PERIPH_3V3` is enabled. Add a temporary/jumper option for bring-up:

```text
PERIPH_EN_FORCE:
  PERIPH_EN -> 3.3V through jumper or 10k
```

This lets the rail turn on during programming even before firmware controls GPIO41 correctly.

### If programmer-powered standalone programming is needed

Add an isolation jumper for ATtiny VCC:

```text
PERIPH_3V3 -- JP_ATtiny_VCC -- ATtiny VCC
UPDI_VCC   -- optional programmer VCC input
```

Normal use:

```text
JP_ATtiny_VCC closed
UPDI_VCC not used as power
```

Standalone programming:

```text
JP_ATtiny_VCC open
programmer powers only ATtiny VCC island
```

This is more flexible but adds complexity and another jumper to manage.

## Small implementation notes

- Add a 470Ω〜1kΩ series resistor on the UPDI signal if desired for protection/debug friendliness.
- Label the header clearly: `VREF / GND / UPDI`, not just `VCC / GND / UPDI`, if the pin is not meant to power the rail.
- If using a programmer that always sources VCC, do not connect its VCC pin to `PERIPH_3V3` unless intentionally powering the whole peripheral rail.

## Current recommendation for VIDEO SIGHT test board

Use target-powered UPDI:

```text
J_UPDI:
  1: PERIPH_3V3 as VREF only
  2: GND
  3: PA0/UPDI
```

Also add a jumper/test option to force `PERIPH_EN` high for bring-up/programming.

If later standalone ATtiny programming becomes necessary, add an ATtiny VCC isolation jumper in the second board revision.
