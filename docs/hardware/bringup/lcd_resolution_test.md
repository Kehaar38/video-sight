# Waveshare 1.83inch LCD Rev2 表示確認

## 目的

Waveshare 1.83inch LCD Module Rev2 / ST7789P の実機表示を確認し、VIDEO SIGHTで使用する表示領域、向き、色、角丸による欠け方を確定する。

## 対象

- MCU: Seeed Studio XIAO ESP32S3 Sense
- LCD: Waveshare 1.83inch LCD Module Rev2
- Controller: ST7789P
- 接続: `docs/hardware/parts_and_pinout.md` のLCD/SPIピンアサイン
- 確認プログラム:
  - `firmware/esp32s3/examples/lcd_resolution_test/`
  - 現行本体側の `firmware/esp32s3/src/main.cpp` にも同等のLCD確認コードを反映済み
- 確認Issue: <https://github.com/Kehaar38/video-sight/issues/6>

## 確認結果

- 表示解像度: `240 x 284`
- 使用向き: `rotation = 0`
- 表示オフセット: `offset = 0, 0`
- 白い外枠線: 4辺すべて表示される
- 色: 赤・緑・青・白の表示に違和感なし
- 角: LCD自体が角丸形状のため、四隅は物理的に欠ける
- 角丸の開始位置: 黄色い20px目盛りの2本目付近からカーブが始まるように見える

## 解釈

白い外枠線が4辺とも見えており、表示文字も `visible=240x284` / `panel=240x284` / `offset=0,0` と読めるため、Rev2の表示領域は仕様通り `240 x 284` と扱ってよい。

角の欠けは解像度・オフセット不整合ではなく、ディスプレイ形状によるものとして扱う。UIや照準表示では、重要情報を四隅の角丸領域へ置かない。

## ファームウェア設定値

```cpp
constexpr int LCD_WIDTH = 240;
constexpr int LCD_HEIGHT = 284;
constexpr int LCD_OFFSET_X = 0;
constexpr int LCD_OFFSET_Y = 0;
constexpr uint8_t LCD_PRODUCT_ROTATION = 0;
```

## UI設計メモ

角丸で欠ける四隅は安全領域外として扱う。少なくとも端から20px程度までは装飾・目盛り程度にとどめ、文字、照準、状態表示、重要なアイコンは内側へ寄せる。
