# Input MCU protocol

This document is the contract between the main ESP32 firmware and the ATmega328P companion firmware.

## Current values

- I2C address: `0x12`
- Read length: 2 bytes

## Byte layout

- Byte 0: `enc_delta` (`int8_t`)
- Byte 1: `buttons_and_status` (`uint8_t`)

## Rule

Bit assignments and packet size should be changed here first, then implemented on both MCU sides.
