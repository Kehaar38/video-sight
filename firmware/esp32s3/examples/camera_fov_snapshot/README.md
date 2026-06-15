# camera_fov_snapshot

XIAO ESP32S3 Sense付属OV3660カメラのFOV確認用PlatformIO例です。

## 目的

- LCDへライブビューを表示する。
- Wakeボタン(D0/GPIO1)で、カメラ取得フレームをmicroSDへBMP保存する。
- 既知寸法物を撮影し、保存BMP上のピクセル数からカメラFOVを測定する。

## 現在の確認済み設定

```text
Camera: OV3660 / RGB565
Frame: XGA 1024x768
LCD: Waveshare 1.83inch Rev2 / ST7789P / 240x284 / rotation 0
LCD live crop: center 101x118 -> 240x284
SD CS: GPIO21
Wake button: D0/GPIO1 active LOW
```

XGA設定は実機で映像表示できた設定です。QXGA/UXGA RGB565はビルドは通りましたが、実機では `capture failed` になりました。

## FOV測定メモ

VGA保存BMPでの実測値:

```text
測定対象: 150 x 300mmの差し金
距離: カメラから約1000mm
保存画像: 640 x 480px
測定対象の画像上サイズ: 48 x 95px

カメラ生FOV:
  水平: 約90.00°
  垂直: 約74.32°
```

XGA設定では、VGAより高いピクセル密度で再測定できる可能性があります。センサー側クロップ/ROI取得が成功した場合は、その方式をこのFOV測定例にも応用し、LCD表示範囲または指定ROIだけを保存する方向を検討します。

## 使い方

```bash
cd firmware/esp32s3/examples/camera_fov_snapshot
pio run
pio run -t upload
pio device monitor
```

microSDの `/fov/` 配下にBMPが保存されます。
