# VIDEO SIGHT 周辺電源EN案

## 対象

DeepSleep時に電源を落とす対象は、XIAO 3V3ピンから供給予定だった外部周辺のみ。

- Waveshare 1.83inch LCD Rev2 VCC
- BNO055 VCC
- ATtiny1616 VCC
- 必要ならLCD BLも同じ周辺電源から供給

XIAO本体、Senseカメラ、microSD等の基板内蔵系はこのENでは制御しない。

## 方針案

- D6/GPIO43をLCD_BL PWMから周辺電源ENへ転用する。
- LCDバックライト調光は初期版では諦め、LCD_BLは周辺電源 `PERIPH_3V3` へ接続する。
- `PERIPH_3V3` はロードスイッチまたはハイサイドスイッチで作る。
- ENにはプルダウンを入れ、起動直後/DeepSleep中/リセット中に周辺電源がOFFになるようにする。

## 評価

利点:

- 予備ピンD11/D12を使わずに済む。
- LCD/IMU/サブMCUをDeepSleep時に確実に落とせる。
- GPIOがHi-ZになってもプルダウンでOFFにできる。
- 電源制御が1本にまとまり、ソフト電源ボタン設計と相性がよい。

注意点:

- LCDバックライトが常時最大輝度になる可能性がある。
- バックライト電流がロードスイッチの負荷に加わる。
- LCD_BLをPERIPH_3V3へ直結してよいか、Waveshare Rev2のBL回路を実機/回路で確認する。
- 周辺電源OFF中にSPI/I2C/GPIOからバックパワーしないよう、DeepSleep前に信号線をLow/Hi-Zへ落とす。
- ATtiny1616も電源OFFになるため、UIサブMCUでWakeする構成ではなく、XIAO直結Wakeボタンで復帰する。

## 推奨回路イメージ

```text
XIAO 3V3
  ↓
Load Switch / High-side Switch
  EN ← D6/GPIO43 + pulldown
  ↓
PERIPH_3V3
  ├─ LCD VCC
  ├─ LCD BL  ※初期案。調光なし
  ├─ BNO055 VCC
  └─ ATtiny1616 VCC
```

## 追加確認

- ロードスイッチの入力電圧範囲: 3.3V対応。
- 出力電流: LCD BL込みの合計電流に十分余裕。
- R_on: 電圧降下が小さいもの。
- EN閾値: ESP32の3.3V GPIOで確実にON/OFFできる。
- OFF時リーク電流: DeepSleep目標に合う。
- 起動順: 起動後にD6を出力Low→周辺初期化直前にHigh。
