# XIAO breakout board notes

## Purpose

Use a small breakout/interposer board and cable between the XIAO ESP32-S3 Sense and the VIDEO SIGHT test board so the XIAO/camera position can be adjusted inside a temporary enclosure.

## BAT- and GND

For the XIAO ESP32-S3 Sense battery connector, `BAT-` should be treated as board `GND`.

Design implication:

- `BAT-` and `GND` can be one schematic net.
- A connector does not need separate logical nets for `BAT-` and `GND`.
- Do not use `BAT-` as an isolated low-side current-sense or low-side switch node; it is not isolated from system ground on the XIAO.

## Cable recommendation

Even though `BAT-` and `GND` are the same logical net, the cable should provide enough ground conductors for return current and signal integrity.

Recommended for the XIAO breakout cable:

- At least one ground conductor paired with the battery/supply path.
- Additional GND pins near SPI/I2C/high-speed/control signal groups if connector pin count allows.
- Prefer not to rely on a single thin GND wire for all XIAO current plus SPI/LCD return current.

Use a single `GND` net in KiCad, but expose multiple physical GND pins on the breakout connector if practical.

## Measurement note

Keep current measurement on the high side as already planned:

```text
18650+ -> SW -> AO3401A -> JP_BAT_CURRENT -> XIAO BAT+
```

Avoid low-side current measurement between `BAT-` and `GND`, because those are common on the XIAO side.
