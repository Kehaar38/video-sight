# VideoSight ハード構成（XIAO コーディング用）

この文書は、ChatGPTログから確定/準確定情報を実装向けに再整理したもの。  
対象MCUは `XIAO ESP32S3 Sense`（PlatformIO: `board = seeed_xiao_esp32s3`）。

## 1. システム構成

- メインMCU: `XIAO ESP32S3 Sense`
- LCD: `Waveshare 1.69" 240x280`（`ST7789V2`, SPI）
- IMU: `BNO055`（I2C, 7bit address `0x28`）
- Input MCU: I2Cスレーブ（7bit address `0x12`）
- ストレージ: microSD（SPI共有）
- 電源: 1セル`18650`、3.3V系で運用

## 2. ピンアサイン（現行案）

| XIAO Pin | GPIO | 用途 |
|---|---:|---|
| D0 | GPIO1 | Wakeボタン入力（DeepSleep復帰） |
| D1 | GPIO2 | LCD_RST |
| D2 | GPIO3 | 予備（必要時に再割当） |
| D3 | GPIO4 | バッテリー分圧ADC入力 |
| D4 | GPIO5 | I2C SDA（BNO055 + Input MCU） |
| D5 | GPIO6 | I2C SCL（BNO055 + Input MCU） |
| D6 | GPIO43 | LCD_BL（PWM調光） |
| D7 | GPIO44 | 予約（将来拡張） |
| D8 | GPIO7 | SPI SCK（LCD/SD共有） |
| D9 | GPIO8 | SPI MISO（主にSD） |
| D10 | GPIO9 | SPI MOSI（LCD/SD共有） |
| D11 | GPIO42 | LCD_DC（JP1カット前提案） |
| D12 | GPIO41 | LCD_CS（JP2カット前提案） |
| On-board | GPIO21 | SD_CS（基板内蔵配線側） |

補足:
- 上記は「実装開始用の現行案」。最終は実機配線確認でFix。
- `D11/D12` 利用は、想定しているJPカット手順とセットで扱う。

## 3. 電源配線（実装前提）

- 18650 -> XIAO `BAT+/-`
- 周辺（LCD/IMU/Input MCU）は基本 `3V3` と `GND` を共通化
- バッテリー監視は分圧して `D3(GPIO4)` へ入力
- 通常OFFは「DeepSleep + 物理電源断スイッチ」併用想定

## 4. バス構成

## 4.1 I2C
- バス: `Wire`（SDA=`D4`, SCL=`D5`）
- 初期クロック: `100kHz`
- 接続先:
  - BNO055 (`0x28`)
  - Input MCU (`0x12`)

## 4.2 SPI（LCD/SD共有）
- SPI信号: `SCK=D8`, `MOSI=D10`, `MISO=D9`
- CS:
  - LCD_CS=`D12`（案）
  - SD_CS=`GPIO21`（on-board）
- 共有ルール（実装前提）:
  - 同時アクセスしない（トランザクション単位で排他）
  - LCD描画中にSD書き込みを割り込ませない

## 5. Input MCU 受信フォーマット（ESP32側解釈）

2バイト固定（I2C read）想定:
- Byte0: `enc_delta`（`int8_t`）
- Byte1: `buttons_and_status`（ビットフィールド）

運用方針:
- エンコーダは距離入力専用
- ボタンは `FRONT/BACK/UP/DOWN` 中心（プロトでは5ボタン由来でもCENTER無視可）
- 長押し判定はESP32側UIで実施（リリース時判定）

注記:
- `buttons_and_status` の厳密ビット割り当ては、Input MCU実装と同時に最終固定すること。

## 6. DeepSleep/起動に関わる最低仕様

- Wake入力: `D0(GPIO1)` を使用
- 推奨入力回路:
  - D0を`INPUT_PULLUP`
  - タクトスイッチでGNDへ落とす
- 起動後の初期化順:
  1. GPIO初期化（BL OFF含む）
  2. I2C初期化（BNO055/Input MCU）
  3. SPI初期化（LCD/SD）
  4. LCD初期化完了後にBL ON

## 7. コーディング用ピン定義テンプレート

```cpp
// ---- XIAO ESP32S3 Sense pin map (VideoSight current plan)
static constexpr int PIN_WAKE_BTN   = 1;   // D0 / GPIO1
static constexpr int PIN_BATT_ADC   = 4;   // D3 / GPIO4
static constexpr int PIN_I2C_SDA    = 5;   // D4 / GPIO5
static constexpr int PIN_I2C_SCL    = 6;   // D5 / GPIO6
static constexpr int PIN_LCD_BL     = 43;  // D6 / GPIO43
static constexpr int PIN_LCD_RST    = 2;   // D1 / GPIO2
static constexpr int PIN_LCD_DC     = 42;  // D11 / GPIO42
static constexpr int PIN_LCD_CS     = 41;  // D12 / GPIO41
static constexpr int PIN_SPI_SCK    = 7;   // D8 / GPIO7
static constexpr int PIN_SPI_MISO   = 8;   // D9 / GPIO8
static constexpr int PIN_SPI_MOSI   = 9;   // D10 / GPIO9
static constexpr int PIN_SD_CS      = 21;  // on-board SD CS

static constexpr uint8_t I2C_ADDR_BNO055    = 0x28;
static constexpr uint8_t I2C_ADDR_INPUT_MCU = 0x12;
```

## 8. 未確定事項（実装時に要FIX）

- Input MCUの `buttons_and_status` ビット割り当て最終版
- SDとLCDのSPI共有時の実効速度上限
- ピーク電流実測に基づく電源マージン
- D7(GPIO44) の最終用途（予約維持か機能割当か）
