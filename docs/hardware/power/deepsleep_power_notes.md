# XIAO ESP32-S3 Sense DeepSleep と周辺電源メモ

## 調査元

- Seeed Studio Wiki: `XIAO ESP32S3 Sense Sleep Modes`
- Espressif ESP-IDF Programming Guide: ESP32-S3 Sleep Modes
- Seeed Studio Wiki: XIAO ESP32-S3 pin multiplexing / 3V3説明

## 結論

- XIAOの3V3端子は、オンボードDCDCレギュレータの出力であり、DeepSleepに入っても自動的に切れるものではないと考える。
- ESP32-S3のDeepSleepでは、CPU、ほとんどのRAM、APB_CLK系のデジタル周辺は電源断/停止する。
- 通常GPIOの出力状態は、DeepSleep中に通常動作時と同じように能動駆動される前提にしない。
- RTC GPIOやGPIO hold機能で一部状態保持は可能だが、VIDEO SIGHTでは「GPIO状態に頼って周辺電源が勝手に落ちる」と考えない方が安全。

## VIDEO SIGHTでの設計判断

- 周辺機器をDeepSleep中に落としたい場合は、3V3直結ではなく、ロードスイッチまたはP-MOSFET高側スイッチ等で周辺電源レールを作る。
- 周辺電源ENは、ESP32が起きている時だけONになる設計にする。
- ESP32がDeepSleepに入りGPIOがHi-Z/未保持になっても、抵抗でENがOFF側へ倒れるようにする。
- 周辺電源OFF中は、SPI/I2C/GPIOからバックパワーしないように、DeepSleep直前に関連GPIOをLowまたはHi-Zへ設定する。
- 物理電源スイッチはバッテリーとXIAO BATを切断する形でよい。この場合、切断中はUSB充電不可。保管/バッテリー交換用の完全OFFとして扱い、使用中の一時停止はDeepSleepに任せる。

## 推奨電源ブロック

```text
保護回路付き18650
  ↓
P-MOSFET逆接保護
  ↓
物理電源スイッチ
  ↓
XIAO BAT
  ↓
XIAO 3V3
  ├─ XIAO本体/カメラ系
  └─ 周辺電源ロードスイッチ
       ├─ LCD VCC / BL系
       ├─ BNO055
       └─ ATtiny1616
```

※ カメラ/Sense拡張基板側の電源がXIAO内部でどう分かれているかは、実機でDeepSleep時電流を測る。
