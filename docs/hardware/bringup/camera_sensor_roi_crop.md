# OV3660 センサー側ROIクロップ試行

## 目的

LCDに表示しない高解像度フレーム全体をESP32側で保持せず、OV3660センサー側で中央ROIだけを出力させる。

狙いは次の通り。

- LCD表示部分だけを扱う。
- VGA/XGAフレームから小さく切り出して拡大するモザイク感を避ける。
- QXGA/UXGA RGB565全体取得時の `capture failed` を避ける。
- SD保存もLCD表示範囲だけにする。

## 現在の試行設定

```text
初期フレームサイズ: CIF 400x296
実出力: OV3660 set_res_raw() による 240x284
Pixel format: RGB565
LCD表示: 240x284 等倍
SD保存: 240x284 BMP
fb_count: 1
```

初期フレームサイズをCIFにしている理由は、esp32-cameraのフレームバッファ確保が `config.frame_size` に依存するため。CIFのRGB565バッファは `400x296x2 = 236800 bytes` で、ROI出力 `240x284x2 = 136320 bytes` より大きい。

## ROIパラメータ

OV3660の4:3フル解像度設定は、esp32-cameraの `ratio_table` で次のようになっている。

```text
max_width/max_height: 2048 x 1536
sensor window: start=(0,0), end=(2079,1547)
offset: 16,6
total: 2300,1564
```

この試行では、フル解像度中央からLCDサイズ相当だけを取り出すため、ダミーマージンを含む `272x296` のタイミング窓を中央に置き、出力を `240x284` にしている。

```text
start: 904,626
end:   1175,921
offset: 16,6
total: 2300,1564
output: 240 x 284
scale: false
binning: false
```

ファーム上では以下で設定する。

```cpp
sensor->set_res_raw(sensor,
  ROI_START_X, ROI_START_Y,
  ROI_END_X, ROI_END_Y,
  ROI_OFFSET_X, ROI_OFFSET_Y,
  ROI_TOTAL_X, ROI_TOTAL_Y,
  LCD_WIDTH, LCD_HEIGHT,
  ROI_SCALE, ROI_BINNING
);
```

## 確認項目

実機では次を確認する。

1. `ROI crop active` がシリアルに出るか。
2. `camera capture failed` にならずライブビューが表示されるか。
3. `fb->width` / `fb->height` が期待通り `240x284` になるか。
4. 色順がこれまで通り正常か。
5. 画角が狭すぎる/広すぎる場合、ROI窓または出力サイズを調整する。
6. Wakeボタンで保存されるBMPが `240x284` になっているか。

## 注意

- `set_res_raw()` は定型 `set_framesize()` より低レベルの設定で、OV3660のレジスタ仕様・ドライバ実装に依存する。
- `240x284` の非標準・縦長出力が受け付けられない可能性がある。
- 失敗時は、まず `320x320` や `400x296` など定型サイズに近いROI出力へ戻して切り分ける。
- センサー側ROIが安定した場合、FOV確認用exampleにも同じ方式を応用し、LCD表示範囲または指定ROIだけを保存する。
