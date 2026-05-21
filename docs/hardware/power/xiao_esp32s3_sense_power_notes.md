# XIAO ESP32-S3 Sense 電源・充電メモ

## 調査元

- Seeed Studio Wiki: Getting Started with Seeed Studio XIAO ESP32-S3 Series
- Seeed Studio Wiki: Pin Multiplexing with Seeed Studio XIAO ESP32-S3 (Sense)
- Seeed Studio公開KiCad/PDF: `202003753_XIAO ESP32S3 Sense_v1.5_SCH_260226`
- SGMICRO product pages: SGM40567, SGM6029

## 確認できた構成

- 充電IC: `SGM40567-4.2XG/TR`
  - 1セルLi-ion/Li-poly系のリニア充電IC。
  - 4.2V版。
  - Seeedの仕様表では、XIAO ESP32-S3 Sense系の充電電流は `100mA(Fast) / 0.9mA(Trickle)` と読める。
  - 充電中/完了表示用の `NCHG` ピンがCHARGE LEDへ接続されている。
- 3.3Vレギュレータ: `SGM6029CYG/TR`
  - SGM6029は同期整流降圧DCDC。
  - SGMICROページでは入力1.95V〜5.5V、低消費、0.6A〜1A級ピーク出力電流とされる。
  - XIAO pin multiplexingページでは3V3ピンから `700mA` 取り出し可能と記載。
- BAT端子: `VBAT`ネット。
  - 3.7V Li-ion/Li-polyセル接続想定。

## 保護機能について

- `SGM40567` は充電管理ICであり、データシート/製品ページ上は充電制御、再充電、トリクル/プリチャージ、熱電流制限などの機能を持つ。
- ただし、XIAO Sense回路中に `DW01`, `FS8205` 等の典型的な1セルLi-ion保護IC/保護FETは確認できなかった。
- したがって、XIAO側だけにセル保護を任せる前提は避ける。過放電・過電流・短絡保護付きセルまたは外部保護基板付き18650を使うのが安全。

## VIDEO SIGHTへの設計上の意味

- 18650は保護回路付き、または保護基板付きホルダーを推奨。
- 充電電流は大きくないため、USB-C充電は可能だが高速充電用途ではない。
- 3V3レールは公称700mAまで使えるとされるが、カメラ+LCD+IMU+ATtiny+SDを載せるため、実測で電圧降下と発熱を確認する。
- LCDバックライトはできればMOSFETでPWM駆動し、3V3レールの余裕と発熱を確認する。
- USB給電中の動作・充電同時運用は発熱確認が必要。
