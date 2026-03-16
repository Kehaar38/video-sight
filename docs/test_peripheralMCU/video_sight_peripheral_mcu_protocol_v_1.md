# VideoSight ⇔ PeripheralMCU 通信プロトコル仕様 v1.0

## 1. 目的

本仕様は **VideoSight 本体と PeripheralMCU(EventHub)** の間の通信インタフェースを定義する。

PeripheralMCU の内部実装（MCU種類、EventNode構成、ボタン処理方法など）には依存せず、
VideoSight 本体は **I2C レジスタと EVENT 線のみ** に依存する設計とする。

この抽象化により、PeripheralMCU の変更や EventNode の拡張があっても
VideoSight 本体のソフトウェア変更を最小化することを目的とする。

---

# 2. システム構成

```
VideoSight (ESP32)
       │
       │ I2C
       │ EVENT
       │
PeripheralMCU (EventHub)
       │
       └ EventNode (UART / EVENT)
```

VideoSight から見える PeripheralMCU の機能は次の3種類のみである。

1. ボタン入力
2. エンコーダ入力
3. 外部イベント

---

# 3. 物理インタフェース

## 3.1 信号線

| 信号 | 方向 | 説明 |
|----|----|----|
| SDA | 双方向 | I2C データ |
| SCL | 入力 | I2C クロック |
| EVENT | PeripheralMCU → VideoSight | 未処理データ通知 |


## 3.2 電圧レベル

- 3.3V ロジック


## 3.3 EVENT 線

- **Active Low**
- 未処理データが存在する間アサート

### アサート条件

EVENT は以下のいずれかで Low になる

- ボタン状態が変化
- エンコーダ差分が存在
- 外部イベントキューにデータが存在

### デアサート条件

EVENT は以下がすべて満たされたとき High に戻る

- ボタン既読
- エンコーダ既読
- 外部イベントキュー空

---

# 4. I2C 基本仕様

## 4.1 バス仕様

| 項目 | 値 |
|----|----|
| モード | I2C スレーブ |
| アドレス | 0x12 |
| 最大クロック | 400kHz |


## 4.2 通信モデル

VideoSight は **ポーリング + EVENT割り込み** モデルで動作する。

1. EVENT Low を検出
2. STATUS レジスタを読み取る
3. 必要なデータを取得
4. ACK レジスタを書き込み


---

# 5. レジスタマップ

## 5.1 概要

| Address | 名前 | R/W | 説明 |
|---|---|---|---|
|0x00|STATUS|R|状態フラグ|
|0x01|BUTTON_STATE|R|ボタン状態|
|0x02|ENC_DELTA|R|エンコーダ差分|
|0x03|EVENT_COUNT|R|イベント数|
|0x04|SRC_PORT|R|イベント送信元|
|0x05|VERSION|R|イベントフォーマット|
|0x06|EVENT_ID|R|イベントID|
|0x07|EVENT_TYPE|R|イベント種類|
|0x08|NODE_TYPE|R|ノード種類|
|0x09|TS_L|R|タイムスタンプ|
|0x0A|TS_H|R|タイムスタンプ|
|0x0B|META0|R|追加情報|
|0x0C|META1|R|追加情報|
|0x0D|FLAGS|R|フラグ|
|0x10|ACK_CLEAR|W|既読通知|


---

# 6. STATUS レジスタ

Address: `0x00`

| Bit | 意味 |
|----|----|
|0|Button unread|
|1|Encoder delta available|
|2|External event available|
|3-7|Reserved|


---

# 7. ボタン状態

Address: `0x01`

| Bit | ボタン |
|----|----|
|0|UP|
|1|DOWN|
|2|FRONT|
|3|BACK|


押下時 `1`

PeripheralMCU 側でデバウンス済み。


---

# 8. エンコーダ

Address: `0x02`

型:

```
int8_t
```

意味:

- 前回既読以降の差分
- ±127 で飽和


### クリック仕様

1 ノッチ = ±1


---

# 9. 外部イベント

イベントは FIFO キューで管理される。

`EVENT_COUNT` は未処理イベント数を表す。


## 9.1 イベント構造

| Field | 説明 |
|----|----|
|SRC_PORT|送信元ノード|
|VERSION|イベントフォーマット|
|EVENT_ID|イベント番号|
|EVENT_TYPE|イベント種別|
|NODE_TYPE|ノード種別|
|TS_L|timestamp|
|TS_H|timestamp|
|META0|追加情報|
|META1|追加情報|
|FLAGS|状態|


---

# 10. ACK / CLEAR

Address: `0x10`

書き込みビット

| Bit | 意味 |
|----|----|
|0|button read|
|1|encoder read|
|2|pop event|


### 使用例

Encoder を読んだ後

```
write 0x10 = 0x02
```

イベントを1件消費

```
write 0x10 = 0x04
```


---

# 11. VideoSight 側推奨処理フロー

```
loop

if EVENT == LOW

    read STATUS

    if button
        read BUTTON_STATE
        write ACK(button)

    if encoder
        read ENC_DELTA
        write ACK(encoder)

    while EVENT_COUNT > 0
        read event fields
        write ACK(pop)
```


---

# 12. エラー処理

PeripheralMCU は以下を保証する

- 不正イベントはキュー投入しない
- EVENT_COUNT と EVENT は整合する
- ENC_DELTA は常に符号付き


---

# 13. タイミング要件

| 項目 | 目標 |
|----|----|
|EVENT 応答|1ms以内|
|I2C 読み取り|<100µs|
|Encoder 更新|割り込み処理|


---

# 14. 拡張方針

本仕様は将来の拡張を前提としている。

### PeripheralMCU 交換

可能:

- ATmega
- RP2040
- ESP32


### EventNode 増設

SRC_PORT により識別可能。


### レジスタ追加

`0x20` 以降を拡張領域とする。


---

# 15. v1.0 完了条件

以下が満たされた時点で v1.0 とする

- ボタン入力取得
- エンコーダ取得
- EventNode1 イベント取得
- EVENT 線通知
- I2C ACK 動作


---

# 16. 設計の要点

VideoSight は PeripheralMCU を **EventHub** として扱う。

本体が依存するのは次の3つのみ。

- EVENT 線
- I2C レジスタ
- データ意味


PeripheralMCU 内部構造は完全に交換可能とする。

これにより VideoSight の将来拡張性と安定性を確保する。

