# TPS22919 検討メモ

## 用途

VIDEO SIGHT の `PERIPH_3V3` 生成用ロードスイッチ候補。

```text
XIAO 3V3
  → TPS22919
  → PERIPH_3V3
      - LCD VCC
      - LCD BL
      - BNO055 VCC
      - ATtiny1616 VCC
```

ENは D6/GPIO43 から制御し、ENにプルダウンを入れて起動直後/DeepSleep中はOFFへ倒す。

## 確認した仕様

TI製品ページより:

- 品名: TPS22919
- 機能: 5.5-V, 1.5-A, 90-mΩ load switch with adjustable output discharge
- Vin: 1.6V〜5.5V
- Imax: 1.5A
- Ron typ: 90mΩ
- Iq typ: 8µA
- Shutdown current typ: 0.002µA
- Features: Inrush current control, Quick output discharge, Short circuit protection, Thermal shutdown
- Soft start: Fixed rise time
- Package image: 6-pin DCK

DCKはTIの小型リード付きSMDパッケージ系で、QFN/BGAではない。手はんだは可能だがかなり小さい。

## 電圧降下・発熱目安

Ron=90mΩとして:

- 0.1A: 9mV / 0.9mW
- 0.2A: 18mV / 3.6mW
- 0.3A: 27mV / 8.1mW
- 0.5A: 45mV / 22.5mW
- 1.0A: 90mV / 90mW
- 1.5A: 135mV / 202.5mW

VIDEO SIGHTの周辺電流が数十〜数百mAなら十分余裕がある。

## 判断

TPS22919はVIDEO SIGHTの周辺電源スイッチとして使える。保護機能と低Ron、低シャットダウン電流があり、3.3Vロードスイッチ用途に合う。

注意:

- パッケージDCKはリード付きだが小さい。試作ではDCK→DIP変換基板等を使うとよい。
- ENピンには外付けプルダウンを入れる。
- Quick output discharge付きなので、OFF時にPERIPH_3V3が速く落ちる。LCD/IMU/ATtiny用途には都合が良い。
- 出力側大容量コンデンサやバックライト突入電流はデータシート推奨に従って確認する。
