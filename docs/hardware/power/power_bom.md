# VIDEO SIGHT 電源周り 部品表ドラフト

このファイルは電源周りだけの抜粋です。最新の全体BOMは次を参照してください。

```text
docs/hardware/test_board_bom.md
```

Source schematic: `hardware/kicad/video_sight_test_board/video_sight_test_board.kicad_sch`

## 前提構成

```text
Protected 18650
  → J1 BATTERY
  → SW1 physical power switch
  → Q1 AO3401A P-MOSFET reverse-polarity protection
  → JP1 BATT_CURRENT
  → XIAO_BAT / XIAO BAT+

XIAO_3V3
  → U1 TPS22919 load switch
  → PERIPH_3V3
      - LCD VCC
      - BNO055 VCC
      - ATtiny1616 VCC through JP2

GPIO41/D12 pad
  → PERIPH_EN
  → U1 TPS22919 ON

D6/GPIO43
  → LCD_BL

XIAO_BAT / +BATT sense
  → R4/R5 divider
  → BATT_ADC
```

## Power-related BOM

| Ref | Qty | 部品 | 値 / 型番 | Footprint | Populate | 用途 | 備考 |
|---|---:|---|---|---|---|---|---|
| J1 | 1 | Battery connector | JST-XH 2pin | `Connector_JST:JST_XH_B2B-XH-A_1x02_P2.50mm_Vertical` | Yes | Battery input | Polarity確認。 |
| SW1 | 1 | Physical power switch | SPST slide switch | `Button_Switch_THT:SW_DIP_SPSTx01_Slide_9.78x4.72mm_W7.62mm_P2.54mm` | Yes | 完全OFF | 実部品寸法/電流定格確認。 |
| Q1 | 1 | P-MOSFET | AO3401A | `Package_TO_SOT_SMD:SOT-23` | Yes | 逆接保護 | Pin map checked: `1=G`, `2=S`, `3=D`。 |
| R1 | 1 | Gate pulldown | 1MΩ | 0805 hand-solder | Yes | Q1 gate-GND | 物理ON時の消費を小さくする。 |
| JP1 | 1 | Current-measure jumper | 2pin header | `PinHeader_1x02_P2.54mm_Vertical` | Yes | Battery current measurement | 通常時はジャンパ短絡。測定時は外して電流計を直列。 |
| U1 | 1 | Load switch | TPS22919 | `@自分用:DCK0006A_L` | Yes | `PERIPH_3V3` control | Custom/local footprint。Gerberで必ず確認。 |
| R2 | 1 | EN pulldown | 100kΩ | 0805 hand-solder | Yes | `PERIPH_EN` default off | Reset/DeepSleep時に周辺電源OFFへ倒す。 |
| R3 | 1 | QOD resistor | 1kΩ | 0805 hand-solder | Yes | TPS22919 QOD tuning | OUT放電用。 |
| C1 | 1 | Input bypass | 1µF | 0805 hand-solder | Yes | TPS22919 IN-GND | U1近傍。 |
| C2 | 1 | Output bypass | 1µF | 0805 hand-solder | Yes | TPS22919 OUT-GND | U1近傍。 |
| C3 | 1 | Peripheral bulk capacitor | 47µF | `@自分用:CAP_FN_B_PAN` | Yes | `PERIPH_3V3` bulk | 極性/サイズ/耐圧確認。 |
| C4 | 1 | XIAO_3V3 bulk capacitor | 47µF | `@自分用:CAP_FN_B_PAN` | Yes | `XIAO_3V3` bulk | 極性/サイズ/耐圧確認。 |
| R4 | 1 | Battery divider upper | 470kΩ | 0805 hand-solder | Yes | Battery ADC | 1%推奨。 |
| R5 | 1 | Battery divider lower | 220kΩ | 0805 hand-solder | Yes | Battery ADC | 1%推奨。 |
| C9 | 1 | ADC smoothing capacitor | 0.047µF | 0805 hand-solder | Yes | `BATT_ADC` smoothing | 高抵抗分圧の読み取り安定化。 |
| TP1 | 1 | Test point | BAT+ | THT pad D1.5/D0.7 | Yes | 電池正極測定 |  |
| TP2, TP7, TP12, TP14 | 4 | Test point | GND | THT pad D1.5/D0.7 | Yes | 測定GND | 複数配置。 |
| TP3 | 1 | Test point | BAT_SW | THT pad D1.5/D0.7 | Yes | スイッチ後測定 |  |
| TP4 | 1 | Test point | XIAO_BAT | THT pad D1.5/D0.7 | Yes | XIAO BAT入力測定 |  |
| TP5 | 1 | Test point | XIAO_3V3 | THT pad D1.5/D0.7 | Yes | XIAO 3.3V測定 |  |
| TP6 | 1 | Test point | PERIPH_EN | THT pad D1.5/D0.7 | Yes | Load-switch EN測定 |  |
| TP9 | 1 | Test point | VBUS | THT pad D1.5/D0.7 | Yes | USB/VBUS観測 |  |
| TP11 | 1 | Test point | PERIPH_3V3 | THT pad D1.5/D0.7 | Yes | 周辺3.3V測定 |  |

## Battery divider

```text
R4 = 470kΩ
R5 = 220kΩ
C9 = 0.047µF
```

Approximate ADC node voltage:

```text
4.2V -> 1.34V
3.7V -> 1.18V
3.0V -> 0.96V
```

Divider current:

```text
4.2V / (470kΩ + 220kΩ) ≈ 6.1µA
```

## Notes

- `R7/R8` optional I2C pull-ups are not included here because they are not power-path parts; see the full BOM.
- `SW2` WAKE was added after ERC review; it is listed in the full BOM.
- `TP10` is now `SPI_MISO`; it is listed in the full BOM.
- Custom/local footprints should be checked in Gerber output before ordering.
