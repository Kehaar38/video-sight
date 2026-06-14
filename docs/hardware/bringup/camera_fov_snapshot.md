# カメラライブ表示・FOV測定スナップショット

## 目的

XIAO ESP32S3 Sense付属カメラのライブ画像をLCDへ表示し、FOV測定用の静止画をmicroSDへ保存する。

## 対象

- MCU: Seeed Studio XIAO ESP32S3 Sense
- Camera: 付属OV3660カメラ
- Camera sensor max: 2048x1536
- LCD: Waveshare 1.83inch LCD Module Rev2 / ST7789P
- LCD表示領域: 240x284, rotation 0
- SD CS: GPIO21
- 撮影ボタン: Wakeボタン D0 / GPIO1, active LOW
- SD初期化: LCDと共有するSPIを `SCK=GPIO7, MISO=GPIO8, MOSI=GPIO9, CS=GPIO21` として明示し、4MHzで初期化する。LovyanGFX側も同じSPI2/FSPIホストで、LCDは4線SPI・MISO有効として設定する。

## 現在の実装

保存する画像は、LCD表示後の画像ではなく、カメラから取得したRGB565フレームをBMPへ変換したもの。

```text
camera frame: QXGA 2048x1536 / RGB565
SD保存: /fov/fov_0000_2048x1536.bmp など
LCD表示: 2048x1536中央から240x284をクロップし、240x284へ等倍表示
```

LCD表示用クロップは、実測FOVから1倍表示に近い中央領域を使う。

```text
source: 2048x1536
crop:  center 240 x 284 相当
crop origin: x=(2048-240)/2, y=(1536-284)/2
result: 240 x 284へ等倍表示
```

実装上は整数演算で、各LCDピクセルに対応する元画像ピクセルをnearest-neighborでサンプリングしている。

## 操作

1. microSDカードを挿入する。
2. ファームを書き込む。
3. LCDにライブ画像が表示されることを確認する。
4. FOV測定対象をカメラ正面に置く。
5. Wakeボタンを押す。
6. `/fov/` 配下にBMPが保存される。

```bash
cd firmware/esp32s3
pio run
pio run -t upload
pio device monitor
```

## FOV測定方法

既知サイズの物体を既知距離に置き、保存BMP上のピクセル幅を数える。

水平FOV:

```text
fraction = object_px / image_width_px
full_width_at_distance = object_width_mm / fraction
horizontal_fov = 2 * atan(full_width_at_distance / (2 * distance_mm))
```

垂直FOVも同様に、物体高さと画像高さピクセルで計算する。

## 実測結果

2026-06-14時点のOV3660付属レンズ・VGA保存BMPでの実測値。

```text
測定対象: 150 x 300mmの差し金
距離: カメラから約1000mm
保存画像: 640 x 480px
測定対象の画像上サイズ: 48 x 95px

水平スケール: 150mm / 48px = 3.125mm/px
垂直スケール: 300mm / 95px = 3.158mm/px

1m地点でのカメラ生フレーム全体の見え幅:
  横: 約2000mm
  縦: 約1516mm

カメラ生FOV:
  水平: 約90.00°
  垂直: 約74.32°
```

この結果から、QXGAでLCD表示を等倍にする場合、ライブビューでは中央 `240 x 284px` をクロップする。これは目の距離約125mmでほぼ1倍になり、150mmでは少し広角寄りになるが、VGAの拡大表示で見えていたモザイク状の粗さを避ける初期値である。これにより、1m地点での見え幅はおおよそ次の値になる。

```text
クロップ: 240 x 284px
1m地点での見え幅: 約234 x 280mm
表示FOV: 水平 約13.37° / 垂直 約15.95°
等倍で1倍になる最大視距離: 約125mm
```

## 1倍表示の目標

LCDサイズ `29.52 x 34.93mm`、目の距離 `150mm` の場合、1倍相当のLCD見かけ画角は次の値。

```text
水平: 約11.24°
垂直: 約13.28°
対角: 約17.34°
```

1m先では、1倍時の見え幅は次の程度。

```text
横: 約197mm
縦: 約233mm
```

保存BMPからカメラ生FOVを測定し、その後に1倍相当になる中央クロップ量を決める。2026-06-14時点ではQXGA `240 x 284px` の等倍中央クロップを初期値とする。

## 注意

- 距離はLCDではなく、カメラレンズから対象物までで測る。
- 対象物はカメラ面と平行に置く。
- 最初はレンズ歪みが少ない中央付近でピクセル数を数える。
- LCDには位置合わせ用に縦長クロップ表示しているが、保存BMPはクロップ前のカメラフレーム。
- ステータス表示は角丸で欠けないよう、左上端ではなく安全領域内に表示する。
- 起動時にSD初期化へ失敗した場合でも、Wakeボタン押下時にSD初期化を一度リトライする。
- 画面が左右反転している場合は、OV3660のセンサー設定で `hmirror` を調整する。
- 上下反転は実機確認済みのため、現在のファームでは `set_vflip(1)` で補正している。
- RGB565のbyte orderは、LCDライブ表示とBMP保存で分けて扱う。LCD表示は `low byte, high byte`、24bit BMP保存は `high byte, low byte` として展開する。
