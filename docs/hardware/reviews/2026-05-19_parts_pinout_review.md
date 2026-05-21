# VIDEO SIGHT 部品構成・ピンアサインレビュー

## 確認した外部仕様

- Seeed XIAO ESP32-S3 Sense wiki上のピン表では、D4=GPIO5/SDA、D5=GPIO6/SCL、D8=GPIO7/SCK、D9=GPIO8/MISO、D10=GPIO9/MOSI。
- Seeed の pin multiplexing ページでは Sense 側の microSD は CS=GPIO21、SCK=D8/GPIO7、MISO=D9/GPIO8、MOSI=D10/GPIO9 と記載されている。Getting Startedページ側のGPIO3記載よりこちらを正とする。
- Waveshare 1.83inch LCD Module は SPI。使用品は Rev2 / ST7789P / 240x284 とする。基板プリントが240x280でも、Rev1からの表示残りとして扱い、まず仕様値240x284で実装する。表示領域は約30.197 x 35.230 mm。

## 主要レビュー

### 良い点

- XIAO ESP32-S3 Sense + 付属DVPカメラ + SPI LCD の構成は、CVBS変換が不要で試作性が高い。
- BNO055をI2C、ATtiny1616 UIサブMCUもI2C候補にする構成はGPIO節約に向く。
- ATtiny1616に4ボタン+ロータリーエンコーダを集約する方針は、メイン側の映像/LCD/IMU処理遅延を抑えやすい。
- D0/GPIO1をWakeボタン専用にするのは役割が明確。
- LCD_BLをPWM可能なGPIO43へ置く案は自然。

### 要確認・修正候補

1. SD CSの割り当て

訂正: Sense拡張ボードのmicroSD CSは GPIO21 とする。Getting Startedページ側にはGPIO3と読める記載があるが、pin multiplexingページではGPIO21で、実機情報とも合う。したがって現在案の「基板内蔵配線 GPIO21 = SPI CS(SD)」は妥当。

影響:

- D2/GPIO3 を LCD_DC として使う案は、SD CSとは衝突しない。
- SDとLCDは SCK/MISO/MOSI を共有し、CSを分ける構成でよい。
- ただしSD使用時はSPIバス帯域をLCD更新と共有するため、同時アクセス時の表示遅延に注意する。

2. D11/D12の扱い

Seeed wikiでは GPIO42/41 はデジタルマイクのCLK/DATAにも使われる。案の通りJPカットで外部使用可能にするならよいが、マイクは使えなくなる前提を明記する。

3. Waveshare LCD revision差

使用するLCDは Rev2 とし、コントローラは ST7789P、解像度は 240x284 として実装する。モジュール上の印字が240x280でも、Rev1由来の印字残りと見なし、まず仕様値を優先する。

実装時の確認:

- ST7789系初期化で動かす
- 論理高さを284にする
- 画面端4pxの扱いを実機で確認する
- もし下端/上端に乱れや非表示が出る場合だけ、実効表示領域を240x280へ落とす

4. I2Cバス

BNO055とATtiny1616を共有するなら、以下を設計メモに入れる。

- BNO055アドレス: 通常0x28または0x29
- ATtiny1616 UIサブMCU: 0x30など、衝突しにくい固定アドレス候補
- プルアップ抵抗はモジュール側搭載分を確認し、合成抵抗が強すぎないようにする
- すべて3.3V駆動に統一

5. BNO055の使い方

BNO055は姿勢推定が楽だが、エアガン周辺では磁気センサが乱れる可能性がある。弾道補正に使う主軸はロール/ピッチとし、ヨーや絶対方位は信用しすぎない。

6. バッテリーADC

D3/GPIO4で分圧測定するなら、ディープスリープ時の常時消費を避けるため、分圧抵抗を高めにするか、P-MOS/N-MOS等で測定時だけ分圧を有効にする設計が望ましい。

## 暫定結論

全体構成は現実的。SD CSは GPIO21 として扱うため、現在の D2/GPIO3=LCD_DC はSDとは衝突しない。LCDは Rev2 / ST7789P / 240x284 として実装し、実機で端4pxの表示を確認する。
