# OV3660 センサー側ROIクロップ試行

## 目的

LCDに表示しない高解像度フレーム全体をESP32側で保持せず、OV3660センサー側で中央ROIだけを出力させる。

狙いは次の通り。

- LCD表示部分だけを扱う。
- VGA/XGAフレームから小さく切り出して拡大するモザイク感を避ける。
- QXGA/UXGA RGB565全体取得時の `capture failed` を避ける。
- SD保存もLCD表示範囲だけにする。

## 失敗した設定

最初に `240x284` の非標準・縦長ROIを試した。

```text
初期フレームサイズ: CIF 400x296
実出力: OV3660 set_res_raw() による 240x284
Pixel format: RGB565
LCD表示: 240x284 等倍
SD保存: 240x284 BMP
fb_count: 1
```

この設定では、シリアル上は次の通り `set_res_raw()` 自体は成功した。

```text
ROI crop active: start=(904,626) end=(1175,921) output=240x284 scale=0 binning=0
Camera ready: psram=yes fb_count=1 init_frame=CIF roi=240x284
```

しかし、その直後に `cam_task` のスタックカナリアで再起動した。

```text
Guru Meditation Error: Core 0 panic'ed (Unhandled debug exception).
Debug exception reason: Stack canary watchpoint triggered (cam_task)
```

そのため、`240x284` の非標準ROIは現時点では危険な設定として扱う。

## 現在の試行設定

次の切り分けとして、ESP32 camera定型サイズにもある `240x240` の正方形ROIへ下げる。

```text
初期フレームサイズ: FRAMESIZE_240X240
実出力: OV3660 set_res_raw() による 240x240
Pixel format: RGB565
LCD表示: 240x284 LCD中央に240x240を等倍表示、上下22pxは黒帯
SD保存: 240x240 BMP
fb_count: 1
```

この設定でライブビューが出れば、`set_res_raw()` による中心ROI出力そのものは使える可能性が高い。次に `320x320` や `240x284` 再調整へ進む。

## 実機確認結果

`240x240` ROI設定では、実機でライブビューが表示された。

確認できたこと:

- LCD表示が出る。
- 上下に黒帯が出る想定通りの表示になる。
- 目から約125mmでおおむね1倍に見える。
- VGA/XGA全体取得からのクロップ表示より、遅延が少なくキビキビ表示される体感がある。
- Wakeボタンで `240x240` の撮影保存ができる。

所感:

- 画質はサイトとして突き詰める段階ではない。
- 全体的に白飛び気味で、ノイズ混じりの低画質カメラのように見える。
- 後で露出・ゲイン・シャープネス・ノイズ低減などのセンサー設定を調整する候補がある。

確認用画像:

```text
dashboard-files/fov_0000_240x240.png
```

この結果から、`set_res_raw()` によるセンサー側ROI出力は有効な方向と判断する。

## ROIパラメータ

OV3660の1:1フル解像度設定は、esp32-cameraの `ratio_table` で次のようになっている。

```text
max_width/max_height: 1536 x 1536
sensor window: start=(256,0), end=(1823,1547)
offset: 16,6
total: 2044,1564
```

現在の試行では、この1:1中央領域の中にダミーマージンを含む `272x252` のタイミング窓を置き、出力を `240x240` にしている。

```text
start: 888,648
end:   1159,899
offset: 16,6
total: 2044,1564
output: 240 x 240
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
  ROI_OUTPUT_WIDTH, ROI_OUTPUT_HEIGHT,
  ROI_SCALE, ROI_BINNING
);
```

## 確認項目

実機では次を確認する。

1. `ROI crop active` がシリアルに出るか。
2. `cam_task` のスタックカナリアで再起動しないか。
3. `camera capture failed` にならずライブビューが表示されるか。
4. `fb->width` / `fb->height` が期待通り `240x240` になるか。
5. 色順がこれまで通り正常か。
6. 画角が狭すぎる/広すぎる場合、ROI窓または出力サイズを調整する。
7. Wakeボタンで保存されるBMPが `240x240` になっているか。

## 注意

- `set_res_raw()` は定型 `set_framesize()` より低レベルの設定で、OV3660のレジスタ仕様・ドライバ実装に依存する。
- `240x284` の非標準・縦長出力は `set_res_raw()` 成功後に `cam_task` が落ちた。
- まず `240x240` の正方形ROIで、センサー側ROIの可否を切り分ける。
- `240x240` が成功した場合は、`320x320` ROIを取得してLCD中央の `240x284` を表示・保存する方法、または `240x284` パラメータの再調整を試す。
- センサー側ROIが安定した場合、FOV確認用exampleにも同じ方式を応用し、LCD表示範囲または指定ROIだけを保存する。
