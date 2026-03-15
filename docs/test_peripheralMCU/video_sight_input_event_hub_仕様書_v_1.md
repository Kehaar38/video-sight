# VideoSight Input / EventHub 仕様書 v1.0

## 1. 文書情報

- 文書名: VideoSight Input / EventHub 仕様書
- 版数: 1.0
- 対象: VideoSight 系入力・外部イベント収集サブシステム
- 状態: 実装確認済み仕様

## 2. 目的

本仕様は、VideoSight 本体に対して以下を提供する入力・外部イベント収集サブシステムを定義する。

- 4 ボタン入力
- ロータリーエンコーダ入力
- 外部 EventNode 由来のイベント入力
- EVENT 線による通知
- I2C レジスタ経由の抽象化されたアクセス手段

本サブシステムは、VideoSight 本体からは MCU 機種や内部実装に依存しない **EventHub** として見えることを目的とする。

## 3. 設計方針

### 3.1 抽象化方針

VideoSight 本体は PeripheralMCU を **I2C 接続の EventHub** として扱う。
本体が依存するのは以下のみとする。

- I2C アドレス
- レジスタマップ
- EVENT 線の論理
- データ意味

PeripheralMCU の内部実装、採用 MCU、EventNode の増減や通信方式変更は、可能な限り PeripheralMCU 側で吸収する。

### 3.2 イベント通知方針

未処理データが存在する場合、PeripheralMCU は VideoSight 本体に対して EVENT 線をアサートする。
VideoSight 本体は EVENT 線を契機に I2C 読み出しを行い、必要なデータを取得・消費する。

### 3.3 EventNode 方針

現行 1.0 では EventNode は 1 系統のみ使用する。
通信方式は **専用 EVENT 線 + 双方向 UART** とする。

EventNode はイベント発生時に自発送信せず、内部でイベントを保持し、PeripheralMCU からの問い合わせに応答する方式を採用する。

## 4. システム構成

### 4.1 構成要素

- VideoSight 本体
- PeripheralMCU
- EventNode1
- 4 ボタン
- ロータリーエンコーダ
- 表示デバイス

### 4.2 現行実装での MCU 構成

- VideoSight 本体: XIAO ESP32 系
- PeripheralMCU: ATmega328P
- EventNode1: ATtiny85

本仕様上は MCU 固有名を本質要件とはしないが、1.0 実装は上記構成を基準とする。

## 5. 役割分担

### 5.1 VideoSight 本体

- EVENT 線監視
- I2C による PeripheralMCU からのデータ取得
- ボタン状態表示
- エンコーダ累計値表示
- 外部イベントのログ・表示・利用

### 5.2 PeripheralMCU

- ボタン読み取り
- ロータリーエンコーダ読み取り
- EventNode1 との双方向 UART 通信
- 外部イベントのキュー管理
- EVENT 線による本体通知
- I2C レジスタの提供

### 5.3 EventNode1

- ローカル入力またはセンサー入力の監視
- イベントの内部保持
- EVENT 線による PeripheralMCU への通知
- PeripheralMCU の GET_EVENT / CLEAR_EVENT に応答

## 6. 物理インタフェース

## 6.1 PeripheralMCU ピン割り当て

### EventNode1 接続

- PD0: EventNode1 TX 受信
- PD1: EventNode1 RX 送信
- PD2: EventNode1 EVENT 入力

### UI 入力

- PD4: BTN_UP
- PD5: BTN_DOWN
- PD6: BTN_FRONT
- PD7: BTN_BACK
- PB0: ENC_A
- PB1: ENC_B

### VideoSight 本体接続

- PB2: EVENT 出力
- PC4: I2C SDA
- PC5: I2C SCL

## 6.2 EventNode1 ピン割り当て

- PB0: UART TX
- PB1: EVENT 出力
- PB2: ローカル入力
- PB3: UART RX
- PB4: デバッグ LED または予備

## 6.3 VideoSight 本体接続

- EVENT 入力 1 本
- I2C SDA / SCL

## 7. 論理インタフェース

## 7.1 EVENT 線

### 7.1.1 PeripheralMCU → VideoSight 本体

- アクティブ Low
- 未処理データが 1 件以上ある間アサート
- 以下のいずれかでアサートされる
  - ボタン未読状態あり
  - エンコーダ未読差分あり
  - 外部イベントキュー非空

### 7.1.2 EventNode1 → PeripheralMCU

- アクティブ Low
- EventNode1 に未処理イベントが保持されている間アサート

## 7.2 I2C

- PeripheralMCU は I2C スレーブとして動作
- 固定アドレスを持つ
- VideoSight 本体は I2C マスタとしてアクセス

## 7.3 UART

### PeripheralMCU ↔ EventNode1

- 双方向 UART
- EventNode1 は自発送信しない
- PeripheralMCU のコマンドでイベント取得

## 8. EventNode UART プロトコル

## 8.1 PeripheralMCU → EventNode1 コマンド

### フレーム形式

- Byte0: CMD_HEADER
- Byte1: CMD
- Byte2: CHECKSUM

### 定義

- CMD_HEADER = 0x55
- CMD_GET_EVENT = 0x12
- CMD_CLEAR_EVENT = 0x13
- CHECKSUM = Byte0 XOR Byte1

## 8.2 EventNode1 → PeripheralMCU イベントフレーム

### フレーム形式

- Byte0: FRAME_HEADER
- Byte1: VERSION
- Byte2: EVENT_ID
- Byte3: EVENT_TYPE
- Byte4: NODE_TYPE
- Byte5: TS_L
- Byte6: TS_H
- Byte7: META0
- Byte8: META1
- Byte9: FLAGS
- Byte10: CHECKSUM

### 定義

- FRAME_HEADER = 0xA5
- VERSION = 0x01
- CHECKSUM = Byte0〜Byte9 の XOR

## 8.3 動作

1. EventNode1 でイベント発生
2. EventNode1 はイベント情報を内部保持
3. EventNode1 は EVENT 線を Low
4. PeripheralMCU は EVENT 検知後、CMD_GET_EVENT を送信
5. EventNode1 は保持中フレームを返送
6. PeripheralMCU は受信成功後にキューへ格納
7. PeripheralMCU は CMD_CLEAR_EVENT を送信
8. EventNode1 は保持を解除し EVENT 線を High に戻す

## 9. PeripheralMCU I2C レジスタ仕様

## 9.1 ステータス

### 0x00 STATUS

- bit0: ボタン未読あり
- bit1: エンコーダ未読あり
- bit2: 外部イベント未読あり

## 9.2 UI 状態

### 0x01 BUTTON_STATE

- bit0: UP
- bit1: DOWN
- bit2: FRONT
- bit3: BACK

### 0x02 ENC_DELTA

- 符号付き 8bit 差分
- ACK でクリアされるまで累積保持

## 9.3 外部イベントキュー先頭

### 0x03 EVENT_COUNT

- 外部イベント件数

### 0x04 SRC_PORT
### 0x05 VERSION
### 0x06 EVENT_ID
### 0x07 EVENT_TYPE
### 0x08 NODE_TYPE
### 0x09 TS_L
### 0x0A TS_H
### 0x0B META0
### 0x0C META1
### 0x0D FLAGS

## 9.4 ACK / CLEAR

### 0x10 ACK_CLEAR

書き込みビット定義:

- bit0: ボタンイベント既読
- bit1: エンコーダ差分既読
- bit2: 外部イベント先頭 1 件 POP

## 10. ボタン仕様

- 4 ボタン構成
- 論理は押下時 1
- PeripheralMCU でデバウンス処理を行う
- VideoSight 本体にはビットマスクで提供する

### ボタンビット定義

- bit0 = UP
- bit1 = DOWN
- bit2 = FRONT
- bit3 = BACK

## 11. エンコーダ仕様

## 11.1 基本方針

- A/B 2 相入力
- PeripheralMCU 側でクォドラチャデコード
- 生遷移ではなく **ノッチ単位** で増減

## 11.2 クリック換算

- クォドラチャステップを途中経過として保持
- `ENC_STEPS_PER_NOTCH = 2` を基準に ±1 へ変換
- 1 ノッチあたり 1 増減とする

## 11.3 出力方針

- PeripheralMCU は未読差分を `ENC_DELTA` に累積保持
- VideoSight 本体は読み出した差分を自身で累積し、`ENC_TOTAL` として扱ってよい

## 12. 外部イベント仕様

## 12.1 イベント ID

- EventNode1 内で単調増加
- 8bit ロールオーバー許容

## 12.2 タイムスタンプ

- 16bit
- EventNode ローカル時刻
- 相対時間差用途を主とする

## 12.3 メタデータ

- META0 / META1 / FLAGS は EventNode 依存の補助情報
- PeripheralMCU は意味解釈を最小限に留め、基本的に透過転送する

## 13. 画面表示仕様（テスト実装）

## 13.1 通常画面

- `BUTTONS`
- ボタンビット一括表示
- `ENC_TOTAL`
- エンコーダ差分累計値表示

## 13.2 外部イベント画面

- EventNode イベント受信時に優先表示
- 1 件あたり 1 秒表示
- 複数件は FIFO 順表示
- 表示後は通常画面へ復帰

## 14. エラー処理方針

- EventNode フレームはヘッダとチェックサムで検証する
- 不正フレームは破棄する
- 外部イベントキュー満杯時は新規投入失敗とする
- キュー空時の POP は無視する

## 15. 非機能要件

### 15.1 安定性

- EventNode 1 系統構成で安定動作すること
- ボタンおよびエンコーダ入力が実使用上安定して読み取れること

### 15.2 拡張性

- PeripheralMCU を将来別 MCU に置き換え可能であること
- VideoSight 本体側 API は I2C レジスタ仕様と EVENT 論理を維持すること
- 将来複数 EventNode に拡張する際も、本体側変更を最小限に抑えること

### 15.3 実装分離

- EventNode 通信方式や内部管理方式は PeripheralMCU に隠蔽する
- VideoSight 本体は EventHub 抽象にのみ依存する

## 16. 1.0 時点の採用判断

- EventNode は 1 系統のみ採用
- EventNode 通信は EVENT + 双方向 UART を採用
- ボタン 4 個 + エンコーダ 1 個を PeripheralMCU で処理
- PeripheralMCU は EventHub として VideoSight 本体へ I2C で公開

## 17. 将来拡張

### 17.1 PeripheralMCU 交換

将来的に以下へ交換可能であることを想定する。

- 上位性能 AVR
- ESP32 系
- RP2040 系
- その他 UART / I2C / 割り込み処理能力を持つ MCU

交換後も以下は維持する。

- EVENT 線論理
- I2C レジスタ仕様
- EventHub としての見え方

### 17.2 EventNode 増設

将来的には EventNode を複数接続する可能性がある。
その場合も本仕様 1.0 の本体側見え方はできる限り維持し、増設は PeripheralMCU 側で吸収する。

## 18. 1.0 完了条件

以下を満たしたため、本仕様を 1.0 とする。

- 4 ボタン状態が期待通り取得できる
- エンコーダが 1 ノッチあたり 1 増減で取得できる
- EventNode1 のイベント取得が安定して行える
- 本体表示でボタン・エンコーダ・外部イベントが期待通り確認できる
- PeripheralMCU を EventHub として抽象化する基本構造が成立している

## 19. 付記

本仕様は現時点の実装確認済み構成を基準とした 1.0 版である。
将来の複数 EventNode 対応、PeripheralMCU 交換、実機 VideoSight 本体統合時は、互換性を維持しつつ 1.1 以降として改訂する。

