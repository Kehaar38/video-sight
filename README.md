# VIDEO SIGHT

VIDEO SIGHT is a compact digital sight prototype built around Seeed Studio XIAO ESP32-S3 Sense.

The project goal is a low-latency, approximately 1x-feeling video sight with camera input, LCD display, IMU-assisted attitude information, and a lightweight ballistic overlay for airsoft use.

## Current hardware direction

- Main MCU: Seeed Studio XIAO ESP32-S3 Sense
- Camera: XIAO ESP32-S3 Sense camera module
- Display: Waveshare 1.83 inch LCD Rev2 / ST7789P / 240x284
- IMU: BNO055 module
- UI sub-MCU: ATtiny1616
- Battery: protected 18650 cell
- microSD CS: GPIO21 on XIAO ESP32-S3 Sense
- Peripheral power switch: TPS22919, controlled by D6/GPIO43
- Reverse polarity protection: AO3401A P-MOSFET

## Repository layout

```text
docs/
  hardware/
    parts_and_pinout.md
    power/
    reviews/
hardware/
  kicad/
firmware/
  esp32s3/
  attiny1616/
```

## Status

Early hardware design. The current focus is the power path and first test PCB.

See:

- `docs/hardware/parts_and_pinout.md`
- `docs/hardware/power/power_bom.md`
- `docs/hardware/power/power_design_notes.md`
- `docs/hardware/reviews/2026-05-20_power_schematic_review.md`
