# 主要部品
## メインMCU
XIAO ESP32-S3 Sense
[Getting Started with Seeed Studio XIAO ESP32-S3 Series | Seeed Studio Wiki](https://wiki.seeedstudio.com/xiao_esp32s3_getting_started/)

## カメラ
XIAO ESP32-S3 Sense付属カメラ / OV3660
- 最大解像度: 2048x1536
- VGA保存BMPでの実測FOV: 水平 約90.00° / 垂直 約74.32° (1m先の150x300mm差し金が48x95px)
- 1倍ライブビュー初期クロップ: QXGAで中央240x284pxを等倍表示。約125mm視距離でほぼ1倍。詳細は `docs/hardware/bringup/camera_fov_snapshot.md`。
[Seeed Studio Wiki](https://wiki.seeedstudio.com/xiao_esp32s3_getting_started/#installation-of-expansion-boards-for-sense)

## ディスプレイ
Waveshare 1.83inch LCD Module Rev2 / ST7789P
- 表示領域: 240x284 (実機確認済み)
- 使用向き: rotation 0
- 角丸形状のため四隅は安全領域外として扱う
[1.83inch LCD Module - Waveshare Wiki](https://www.waveshare.com/wiki/1.83inch_LCD_Module)

## IMU
BNO055モジュール
[BNO055使用 9軸センサーフュージョンモジュールキット: 計測器・センサー・ロガー 秋月電子通商-電子部品・ネット通販](https://akizukidenshi.com/catalog/g/g116996/)

## サブMCU
ATtiny1616
[AVRマイコン ATTINY1616-SFR: 半導体 秋月電子通商-電子部品・ネット通販](https://akizukidenshi.com/catalog/g/g130947/)

## 入力装置
- サブMCU配下
	- 4xタクトスイッチ
	- 1xロータリーエンコーダー (ボタンなし)
- メインMCU配下
	- 1xタクトスイッチ

## バッテリー
18650バッテリー
XIAOのバッテリー端子に接続

# ピンアサイン

|XIAO Pin|GPIO|用途|
|---|--:|---|
|D0|GPIO1|Wakeボタン (DeepSleep復帰)|
|D1|GPIO2|LCD_RST|
|D2|GPIO3|LCD_DC|
|D3|GPIO4|バッテリー分圧ADC入力|
|D4|GPIO5|I2C SDA (IMU/サブMCU)|
|D5|GPIO6|I2C SCL (IMU/サブMCU)|
|D6|GPIO43|LCD_BL (PWM調光)|
|D7|GPIO44|SPI CS (LCD)|
|D8|GPIO7|SPI SCK (LCD/SD共有)|
|D9|GPIO8|SPI MISO (SD)|
|D10|GPIO9|SPI MOSI (LCD/SD共有)|
|D11|GPIO42|予備/背面パッド引き出し (マイク用ジャンパー注意)|
|D12|GPIO41|PERIPH_EN (TPS22919周辺電源制御、背面パッド引き出し)|
|(基板内蔵配線)|GPIO21|SPI CS (SD、実機確認済み)|