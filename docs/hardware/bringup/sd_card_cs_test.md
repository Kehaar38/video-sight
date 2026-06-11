# XIAO ESP32S3 Sense microSD CS確認

## 目的

XIAO ESP32S3 Sense のmicroSDカードスロットで使用する CS ピンを、実機テストで確定する。

Seeed Studio Wiki では、XIAO ESP32S3 Sense のmicroSD初期化例として `SD.begin(21)` が示されている一方、Round Display 利用時は `D2` を渡す旨の記述もある。VIDEO SIGHTでは、XIAO ESP32S3 Sense基板内蔵のmicroSDスロットを対象として確認した。

## 確認結果

- 対象: Seeed Studio XIAO ESP32S3 Sense
- 対象スロット: XIAO ESP32S3 Sense 基板内蔵microSDスロット
- microSD CS: `GPIO21`
- Arduino / PlatformIO 初期化: `SD.begin(21)`
- 実機確認: 初期化、ルートディレクトリ一覧、テストファイル書き込み、読み戻しに成功
- `D2/GPIO3` は VIDEO SIGHT のmicroSD CSとしては使用しない

## テストプログラム

保存場所:

```text
firmware/esp32s3/examples/sd_cs_test/
```

実行例:

```bash
cd firmware/esp32s3/examples/sd_cs_test
pio run
pio run -t upload
pio device monitor
```

`src/main.cpp` 冒頭の `SD_CS_PIN` を `21` または `3` に手動変更して比較できる。VIDEO SIGHTでは `21` が正。

```cpp
const int SD_CS_PIN = 21;
```

## 参考

- [Sense版のMicroSDカード | Seeed Studio Wiki](https://wiki.seeedstudio.com/ja/xiao_esp32s3_sense_filesystem/)
