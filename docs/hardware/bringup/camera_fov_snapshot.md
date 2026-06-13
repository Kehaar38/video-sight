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
camera frame: VGA 640x480 / RGB565
SD保存: /fov/fov_0000_640x480.bmp など
LCD表示: 640x480中央から左右をクロップし、240x284へ縮小表示
```

LCD表示用クロップは、4:3のカメラ画像をLCDの縦長比率 `240:284` に合わせる。

```text
source: 640x480
crop:  center 405-406 x 480 相当
display: 240x284
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

## 1倍表示の目標

LCDサイズ `29.52 x 34.93mm`、目の距離 `60mm` の場合、1倍相当のLCD見かけ画角は次の値。

```text
水平: 約27.64°
垂直: 約32.46°
対角: 約41.72°
```

1m先では、1倍時の見え幅は次の程度。

```text
横: 約492mm
縦: 約582mm
```

保存BMPからカメラ生FOVを測定し、その後に1倍相当になる中央クロップ量を決める。

## 注意

- 距離はLCDではなく、カメラレンズから対象物までで測る。
- 対象物はカメラ面と平行に置く。
- 最初はレンズ歪みが少ない中央付近でピクセル数を数える。
- LCDには位置合わせ用に縦長クロップ表示しているが、保存BMPはクロップ前のカメラフレーム。
- ステータス表示は角丸で欠けないよう、左上端ではなく安全領域内に表示する。
- 起動時にSD初期化へ失敗した場合でも、Wakeボタン押下時にSD初期化を一度リトライする。
- 画面が左右反転している場合は、OV3660のセンサー設定で `hmirror` を調整する。
- 上下反転は実機確認済みのため、現在のファームでは `set_vflip(1)` で補正している。
- RGB565のbyte orderは実機表示の色化け確認後、`low byte, high byte` として読む設定にしている。
