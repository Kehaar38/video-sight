# ATmega328P companion firmware

This subproject is the firmware for the small processing MCU used by VideoSight.

## Purpose

- Scan buttons and encoder
- Expose the result to the main ESP32 MCU over I2C
- Keep low-level input handling separate from the main UI/application firmware

## Layout

- `platformio.ini`: build target for the companion MCU
- `src/main.cpp`: firmware entry point
- `include/`: companion-only headers

## Recommended operation

- Keep the main ESP32 firmware in the repository root
- Keep the ATmega328P firmware in this subproject
- Define the ESP32 <-> ATmega328P protocol in `docs/`

## Build

Open this folder as a separate PlatformIO project, or run PlatformIO with this directory as the project root.

