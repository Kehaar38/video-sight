# VIDEO SIGHT 周辺電源EN案

## 対象

DeepSleep時に電源を落とす対象は、XIAO 3V3ピンから供給予定だった外部周辺のみ。

- Waveshare 1.83inch LCD Rev2 VCC
- BNO055 VCC
- ATtiny1616 VCC

XIAO本体、Senseカメラ、microSD等の基板内蔵系はこのENでは制御しない。

## 現在の方針

- `PERIPH_3V3` はTPS22919ロードスイッチで作る。
- `PERIPH_EN` は GPIO41 / D12 背面パッドから制御する。
- D6 / GPIO43 は `LCD_BL` に戻し、LCDバックライトPWM調光用として使う。
- ENには100kΩ程度のプルダウンを入れ、起動直後/DeepSleep中/リセット中に周辺電源がOFFになるようにする。

## 評価

利点:

- D6/GPIO43をLCD_BLに戻せるため、バックライト電流と輝度を実測・調整できる。
- LCD/IMU/サブMCUをDeepSleep時に確実に落とせる。
- GPIOがHi-ZになってもプルダウンでOFFにできる。
- 周辺電源制御が1本にまとまり、ソフト電源ボタン設計と相性がよい。

注意点:

- GPIO41/D12はXIAO背面パッド側なので、ブレイクアウトボードで確実に引き出す必要がある。
- XIAOのカメラシールド/マイク用ジャンパーとの関係を確認する。
- 周辺電源OFF中にSPI/I2C/GPIOからバックパワーしないよう、DeepSleep前に信号線をLow/Hi-Zへ落とす。
- ATtiny1616も電源OFFになるため、UIサブMCUでWakeする構成ではなく、XIAO直結Wakeボタンで復帰する。

## 推奨回路イメージ

```text
XIAO_3V3
  ↓
TPS22919 Load Switch
  EN ← GPIO41 / D12 pad + pulldown
  ↓
PERIPH_3V3
  ├─ LCD VCC
  ├─ BNO055 VCC
  └─ ATtiny1616 VCC

D6 / GPIO43
  ↓
LCD_BL
```

## 追加確認

- ロードスイッチの入力電圧範囲: 3.3V対応。
- 出力電流: LCD/IMU/ATtinyの合計電流に十分余裕。
- R_on: 電圧降下が小さいもの。
- EN閾値: ESP32の3.3V GPIOで確実にON/OFFできる。
- OFF時リーク電流: DeepSleep目標に合う。
- 起動順: 起動後にGPIO41を出力Low→周辺初期化直前にHigh。
