# VIDEO SIGHT schematic review 2026-05-20 power draft

## Reviewed images

- XIAO ESP32-S3 Sense symbol / main power nets
- 18650 + SW + AO3401A reverse protection
- Battery divider
- TPS22919 peripheral load switch

## Findings

### OK: AO3401A reverse protection

Current schematic shows:

```text
18650+ → SW → AO3401A pin3
AO3401A pin2 → +BATT
AO3401A pin1 → 1MΩ → GND
```

This matches the intended direction if the KiCad AO3401A symbol uses common pinout:

- pin1 = Gate
- pin2 = Source
- pin3 = Drain

Action: verify against the exact Digi-Key/AOSMD AO3401A datasheet and footprint before PCB.

### OK: Battery divider

Current schematic:

```text
+BATT → 470kΩ → ADC node → 220kΩ → GND
ADC node → 0.047µF → GND
```

This is good. `+BATT` must mean the protected/switched node after AO3401A, not raw battery.

Calculation:

- 4.2V → about 1.34V at ADC
- divider current at 4.2V → about 6.1µA
- Rth ≈ 150kΩ, C=0.047µF gives τ≈7ms, 5τ≈35ms

### Important: TPS22919 output net name

The TPS22919 output is currently shown with a `+3V3` power symbol. This is dangerous/confusing because KiCad power symbols are global. If any other `+3V3` appears on the unswitched XIAO 3V3 rail, it will short TPS22919 IN and OUT and bypass the load switch.

Action: rename nets:

```text
XIAO 3V3 pin / TPS IN  = XIAO_3V3 or 3V3_RAW
TPS OUT                = PERIPH_3V3
```

Do not use the same global `+3V3` symbol on both sides of TPS22919.

### OK: TPS22919 EN pulldown

`EN → 100kΩ → GND` is good. D6/GPIO43 drives the same EN net.

### QOD / R_QOD

TPS22919 QOD is optional but useful.

Datasheet behavior:

- QOD open: output discharge disabled.
- QOD tied to OUT directly: fastest discharge through internal QOD path, about 24Ω typ internal resistance.
- QOD tied to OUT through external R_QOD: adjustable/slower output discharge.

For VIDEO SIGHT, using QOD is recommended so `PERIPH_3V3` collapses reliably before DeepSleep.

Suggested population options:

- R_QOD = 0Ω: fastest discharge. With about 48µF output capacitance, initial discharge current roughly 3.3V/24Ω ≈ 137mA, τ≈1.2ms.
- R_QOD = 100Ω: gentler, initial roughly 27mA, 5τ≈30ms for 48µF.
- R_QOD = 1kΩ: very gentle, initial roughly 3.2mA, 5τ≈250ms for 48µF.
- DNP/open: disables QOD; not preferred if peripherals need clean power reset.

Recommendation: place R_QOD footprint. For first test, populate 100Ω or 1kΩ. Use 0Ω if fast discharge/reset is needed.

### Missing later, expected

- Current measurement jumper(s)
- Battery holder footprint
- Test points
- PERIPH_3V3 bulk capacitor if not on another sheet
- Local decoupling for LCD/IMU/ATtiny modules

## Summary

No fatal issue in the shown draft, but change TPS22919 OUT from global `+3V3` to a distinct `PERIPH_3V3` net before continuing. Keep R_QOD footprint; it is optional electrically, but useful for controlled shutdown.
