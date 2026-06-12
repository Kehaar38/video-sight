# LCD解像度確認テスト

Waveshare 1.83inch LCD Module Rev2 / ST7789P の動作確認と、表示可能領域の解像度確認用PlatformIO example。

## 対象

- MCU: Seeed Studio XIAO ESP32S3 Sense
- LCD: Waveshare 1.83inch LCD Module Rev2
- Controller: ST7789P
- 確定表示領域: 240 x 284
- 確定使用向き: rotation 0
- 確定オフセット: 0,0

## 実行

```bash
cd firmware/esp32s3/examples/lcd_resolution_test
pio run
pio run -t upload
pio device monitor
```

## 確認ポイント

起動後、次の表示を約3.5秒ごとに切り替える。

- rotation 0/1/2/3 の解像度確認パターン
- 赤、緑、青、白の全画面塗りつぶし

確認すること:

- 白い外枠が4辺すべて見える
- 4隅の `TL` / `TR` / `BL` / `BR` ブロックは、LCDの角丸形状によって角だけ欠ける
- rotation 0/2 で `visible=240x284` と表示される
- rotation 1/3 で `visible=284x240` と表示される
- 色順が自然で、赤・緑・青が入れ替わっていない

## 実機確認結果

Issue #6 の写真で確認済み。

- 白い枠線は4辺とも見えている
- 解像度は `240 x 284` で確定
- VIDEO SIGHTでは `rotation 0` を使用する
- `offset=0,0` でよい
- 角丸ディスプレイのため四隅は物理的に欠ける
- 黄色い20px目盛りの2本目付近からカーブが始まるように見える
- 色表示に違和感はない

## オフセット調整

表示が上下にずれる、外枠が欠ける、余白が片側に寄る場合は、`src/main.cpp` 冒頭付近の値を変更して再ビルドする。

```cpp
constexpr int LCD_OFFSET_X = 0;
constexpr int LCD_OFFSET_Y = 0;
```

ST7789系では縦方向オフセットとして `0`, `20`, `36`, `40` 付近が候補になる。VIDEO SIGHTで実機確認後、確定値を本体ファーム側へ反映する。
