# VIDEO SIGHT test board BOM

Source schematic: `hardware/kicad/video_sight_test_board/video_sight_test_board.kicad_sch`

Updated after ERC fixes:

- `SW2` WAKE switch added.
- `TP10` corrected to `SPI_MISO`.
- R7/R8 optional I2C pull-ups are marked DNP in KiCad.

## Board-mounted parts

| Ref | Qty | Part / value | Footprint | Populate | Notes |
|---|---:|---|---|---|---|
| C1, C2 | 2 | 1µF | `Capacitor_SMD:C_0805_2012Metric_Pad1.18x1.45mm_HandSolder` | Yes | TPS22919 input/output local capacitors. X7R recommended. |
| C3, C4 | 2 | 47µF | `@自分用:CAP_FN_B_PAN` | Yes | Bulk capacitors. Custom/local footprint; verify polarity and physical size before order. |
| C5, C6, C7, C8 | 4 | 0.1µF | `Capacitor_SMD:C_0805_2012Metric_Pad1.18x1.45mm_HandSolder` | Yes | Local decoupling. C8 is on the `UPDI_VCC` / ATtiny VCC side of JP2. |
| C9 | 1 | 0.047µF | `Capacitor_SMD:C_0805_2012Metric_Pad1.18x1.45mm_HandSolder` | Yes | Battery ADC smoothing capacitor. |
| J1 | 1 | BATTERY connector | `Connector_JST:JST_XH_B2B-XH-A_1x02_P2.50mm_Vertical` | Yes | Battery input connector. Confirm mating cable/housing and current rating. |
| J2 | 1 | XIAO breakout board connector | `Connector_IDC:IDC-Header_2x10_P2.54mm_Vertical` | Yes | Keyed IDC cable connection. Confirm key direction and pin 1 before PCB order. |
| J3 | 1 | LCD connector | `Connector_PinHeader_2.54mm:PinHeader_1x08_P2.54mm_Vertical` | Yes | For Waveshare LCD cable/header. Bundled cable can be reversed; final build may need keyed/custom cable. |
| J4 | 1 | BNO055 module socket/header | `Package_DIP:DIP-8_W7.62mm_Socket` | Yes | Fits 2x4 / 2.54mm / 7.62mm row spacing module if measured module matches. |
| J5 | 1 | UPDI header | `Connector_PinHeader_2.54mm:PinHeader_1x03_P2.54mm_Vertical` | Yes | `UPDI_VCC`, GND, PA0/UPDI programming header. |
| J6, J7 | 2 | ATtiny1616 breakout headers | `Connector_PinHeader_2.54mm:PinHeader_1x08_P2.54mm_Vertical` | Yes | Expose ATtiny pins for jumper experiments. |
| J8 | 1 | INPUT_DEVICE header | `Connector_PinHeader_2.54mm:PinHeader_1x06_P2.54mm_Vertical` | Yes | Buttons/encoder external wiring point. |
| JP1 | 1 | BATT_CURRENT jumper | `Connector_PinHeader_2.54mm:PinHeader_1x02_P2.54mm_Vertical` | Yes | Remove jumper to insert ammeter for battery current measurement. |
| JP2 | 1 | ATTINY_VCC jumper | `Connector_PinHeader_2.54mm:PinHeader_1x02_P2.54mm_Vertical` | Yes | Closed for normal operation; open for programmer-powered ATtiny-only mode. |
| Q1 | 1 | AO3401A P-MOSFET | `Package_TO_SOT_SMD:SOT-23` | Yes | Reverse-polarity protection. Pin map checked: `1=G`, `2=S`, `3=D`. |
| R1 | 1 | 1MΩ | `Resistor_SMD:R_0805_2012Metric_Pad1.20x1.40mm_HandSolder` | Yes | AO3401A gate pulldown. |
| R2 | 1 | 100kΩ | `Resistor_SMD:R_0805_2012Metric_Pad1.20x1.40mm_HandSolder` | Yes | TPS22919 `PERIPH_EN` pulldown. |
| R3 | 1 | 1kΩ | `Resistor_SMD:R_0805_2012Metric_Pad1.20x1.40mm_HandSolder` | Yes | TPS22919 QOD resistor. |
| R4 | 1 | 470kΩ | `Resistor_SMD:R_0805_2012Metric_Pad1.20x1.40mm_HandSolder` | Yes | Battery ADC divider upper resistor. 1% recommended. |
| R5 | 1 | 220kΩ | `Resistor_SMD:R_0805_2012Metric_Pad1.20x1.40mm_HandSolder` | Yes | Battery ADC divider lower resistor. 1% recommended. |
| R6 | 1 | 470Ω | `Resistor_SMD:R_0805_2012Metric_Pad1.20x1.40mm_HandSolder` | Yes | PA0/UPDI series resistor. |
| R7, R8 | 2 | 4.7kΩ | `Resistor_SMD:R_0805_2012Metric_Pad1.20x1.40mm_HandSolder` | DNP by default | Optional I2C pull-ups to `PERIPH_3V3`. Populate only if BNO055/module pull-ups are insufficient. |
| SW1 | 1 | SW_BATT power switch | `Button_Switch_THT:SW_DIP_SPSTx01_Slide_9.78x4.72mm_W7.62mm_P2.54mm` | Yes | Physical battery power switch. Confirm exact part. |
| SW2 | 1 | WAKE push button | `Button_Switch_THT:SW_PUSH_6mm` | Yes | Added after ERC review. Pulls `WAKE` to GND. |
| SW3, SW4, SW5, SW6 | 4 | UP / DOWN / FRONT / BACK push buttons | `Button_Switch_THT:SW_PUSH_6mm` | Yes | ATtiny input buttons; firmware enables internal pull-ups. |
| SW7 | 1 | Rotary encoder | `Rotary_Encoder:RotaryEncoder_Alps_EC12E_Vertical_H20mm` | Yes | Confirm exact encoder mechanical dimensions and shaft style. |
| TP1 | 1 | BAT+ test point | `TestPoint:TestPoint_THTPad_D1.5mm_Drill0.7mm` | Yes | Battery positive before switch. |
| TP2, TP7, TP12, TP14 | 4 | GND test points | `TestPoint:TestPoint_THTPad_D1.5mm_Drill0.7mm` | Yes | Probe/logic analyzer ground points. |
| TP3 | 1 | BAT_SW test point | `TestPoint:TestPoint_THTPad_D1.5mm_Drill0.7mm` | Yes | Battery after physical switch, before protection MOSFET. |
| TP4 | 1 | XIAO_BAT test point | `TestPoint:TestPoint_THTPad_D1.5mm_Drill0.7mm` | Yes | Protected battery input to XIAO BAT. |
| TP5 | 1 | XIAO_3V3 test point | `TestPoint:TestPoint_THTPad_D1.5mm_Drill0.7mm` | Yes | Unswitched XIAO 3.3V rail. |
| TP6 | 1 | PERIPH_EN test point | `TestPoint:TestPoint_THTPad_D1.5mm_Drill0.7mm` | Yes | Load switch enable from GPIO41/D12 pad. |
| TP8 | 1 | GPIO42 test point | `TestPoint:TestPoint_THTPad_D1.5mm_Drill0.7mm` | Yes | Spare XIAO backside pad / D11. |
| TP9 | 1 | VBUS test point | `TestPoint:TestPoint_THTPad_D1.5mm_Drill0.7mm` | Yes | XIAO USB/VBUS observation. |
| TP10 | 1 | SPI_MISO test point | `TestPoint:TestPoint_THTPad_D1.5mm_Drill0.7mm` | Yes | Corrected from MOSI to MISO after ERC review. |
| TP11 | 1 | PERIPH_3V3 test point | `TestPoint:TestPoint_THTPad_D1.5mm_Drill0.7mm` | Yes | Switched peripheral 3.3V rail. |
| TP13 | 1 | IMU_2V8_OUT test point | `TestPoint:TestPoint_THTPad_D1.5mm_Drill0.7mm` | Yes | Observe-only module rail/test point. Do not drive. |
| TP15 | 1 | IMU_INT test point | `TestPoint:TestPoint_THTPad_D1.5mm_Drill0.7mm` | Yes | BNO055 interrupt observation. |
| TP16 | 1 | IMU_RESET test point | `TestPoint:TestPoint_THTPad_D1.5mm_Drill0.7mm` | Yes | BNO055 reset observation/control. |
| U1 | 1 | TPS22919 load switch | `@自分用:DCK0006A_L` | Yes | Custom/local footprint. Verify pin map and package before order. |
| U2 | 1 | ATTINY1616-SN | `Package_SO:SOIC-20W_7.5x12.8mm_P1.27mm` | Yes | ATtiny1616 sub-MCU in SOIC-20W. |

## DNP / optional parts

| Ref | Qty | Part | Reason |
|---|---:|---|---|
| R7, R8 | 2 | 4.7kΩ I2C pull-ups | BNO055 module likely has pull-ups. Leave open first; populate only if bus idle/rise time needs it. |

## Off-board / module parts to prepare

These are needed for the prototype but are not all represented as board-mounted KiCad parts.

| Item | Qty | Notes |
|---|---:|---|
| Protected 18650 cell | 1 | Use protected cell as project policy. |
| 18650 holder / battery lead with JST-XH mate | 1 | Must match J1 polarity and protected-cell length. |
| XIAO ESP32-S3 Sense | 1 | Main MCU/camera module side. |
| XIAO breakout board | 1 | Separate breakout/interposer board for camera positioning. |
| Keyed 2x10 IDC cable | 1 | Obtain before PCB order to verify key direction and pin mapping. |
| Waveshare 1.83 inch LCD Rev2 | 1 | ST7789P 240x284 module. Confirm `LCD_BL` behavior. |
| LCD cable / custom keyed cable | 1 | Bundled 2.54mm socket cable is reversible; final build may need keyed/custom cable. |
| AE-BNO055-BO / BNO055 module | 1 | Mounts to J4 if row spacing matches. |
| Jumper shunts | 2+ | For JP1 BATT_CURRENT and JP2 ATTINY_VCC. |
| UPDI programmer | 1 | For ATtiny1616 programming. Use JP2 correctly when programmer supplies VCC. |

## Pre-order checks

- Run ERC again after the BOM update and footprint changes.
- Confirm all DNP parts are exported correctly by KiCad BOM settings.
- Verify custom/local footprints `@自分用:CAP_FN_B_PAN` and `@自分用:DCK0006A_L` in generated Gerbers.
- Confirm IDC cable key orientation and pin 1 before PCB order.
- Confirm LCD cable orientation and whether final hardware needs a keyed/custom cable.
- Confirm `LCD_BL` is safe to drive from GPIO or add a driver if the module exposes raw LED current path.
