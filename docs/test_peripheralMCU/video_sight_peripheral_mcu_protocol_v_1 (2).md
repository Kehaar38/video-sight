# VideoSight ⇔ PeripheralMCU 通信プロトコル仕様 v1.0

## 1. 文書目的

本ドキュメントは **VideoSight 本体と PeripheralMCU 間の通信プロトコル**のみを定義する。

PeripheralMCU 内部実装（EventNode 接続方式、ボタン読み取り方法、エンコーダ処理など）は本仕様の対象外とする。

VideoSight 本体は PeripheralMCU を **EventHub デバイス**として扱う。


---

# 2. 通信構成

VideoSight と PeripheralMCU の通信は以下の2つで構成される。

| 信号 | 用途 |
|-----|------|
| I2C | データ取得 |
| EVENT | データ到着通知 |

VideoSight は **I2Cマスタ**、PeripheralMCU は **I2Cスレーブ**として動作する。


---

# 3. EVENT 線

## 3.1 役割

PeripheralMCU は未処理データが存在する場合、EVENT線をアサートする。

VideoSight は EVENT をトリガとして I2C 読み出しを行う。


## 3.2 論理

| 状態 | 意味 |
|----|----|
| HIGH | 未処理データなし |
| LOW | 未処理データあり |


## 3.3 EVENT が LOW になる条件

以下のいずれかが存在する場合。

- 未読ボタンイベント
- 未読エンコーダ差分
- 未読外部イベント


## 3.4 EVENT 解除条件

以下がすべて消費された場合。

- ボタン状態取得
- エンコーダ差分取得
- 外部イベントキュー空


---

# 4. I2C 基本仕様

| 項目 | 内容 |
|----|----|
| モード | I2C Slave |
| アドレス | 0x32 |
| 転送方式 | レジスタマップ |
| エンディアン | Little Endian |


---

# 5. レジスタマップ

## 5.1 STATUS

アドレス

```
0x00
```

ビット定義

| bit | 意味 |
|----|----|
|0|ボタンイベントあり|
|1|エンコーダ差分あり|
|2|外部イベントあり|


---

## 5.2 BUTTON_STATE

```
0x01
```

| bit | ボタン |
|----|----|
|0|UP|
|1|DOWN|
|2|FRONT|
|3|BACK|

押下時 = 1


---

## 5.3 ENCODER_DELTA

```
0x02
```

型

```
int8
```

仕様

- 未読差分を累積
- VideoSight が取得すると0へリセット


---

# 6. 外部イベント

PeripheralMCU は外部イベントを **FIFOキュー**で管理する。


## 6.1 EVENT_COUNT

```
0x03
```

未処理イベント数


---

## 6.2 イベントデータ

|アドレス|内容|
|---|---|
|0x04|SRC_PORT|
|0x05|VERSION|
|0x06|EVENT_ID|
|0x07|EVENT_TYPE|
|0x08|NODE_TYPE|
|0x09|TIMESTAMP_L|
|0x0A|TIMESTAMP_H|
|0x0B|META0|
|0x0C|META1|
|0x0D|FLAGS|


---

# 7. ACK / CLEAR

アドレス

```
0x10
```

VideoSight が書き込みを行う。


ビット定義

|bit|意味|
|---|---|
|0|ボタン既読|
|1|エンコーダ既読|
|2|イベントPOP|


例

```
0x04
```

イベント1件を消費


---

# 8. 推奨ポーリング手順

VideoSight は以下の順序で処理する。

```
1 EVENT LOW 検出
2 STATUS 読み取り

3 ボタン取得
4 エンコーダ取得

5 EVENT_COUNT 確認
6 イベント取得
7 POP
```


---

# 9. 実装例

### EVENT 割り込みハンドラ

```
if(EVENT == LOW)
    readStatus();
```


### 処理例

```
status = i2cRead(0x00);

if(status & 1)
    buttons = i2cRead(0x01);

if(status & 2)
    enc += i2cRead(0x02);

if(status & 4)
{
    count = i2cRead(0x03);

    while(count--)
    {
        readEvent();
        i2cWrite(0x10,0x04);
    }
}
```


---

# 10. タイミング要件

|項目|推奨値|
|---|---|
|I2C速度|100kHz〜400kHz|
|イベント処理遅延|<10ms|
|EVENT反応|<1ms|


---

# 11. エラー処理

VideoSight は以下を考慮する。


### I2Cエラー

- 再試行


### EVENT不整合

STATUS を再取得


### 不正イベント

イベント破棄


---

# 12. 本プロトコルの設計意図

本仕様は以下を目的として設計されている。


### ① MCU交換耐性

PeripheralMCU を将来

- AVR
- RP2040
- ESP32

へ置き換えても本体側コードを変更しない。


### ② EventNode構成変更耐性

EventNode

- UART
- I2C
- SPI

等の変更を PeripheralMCU 内で吸収する。


### ③ 本体コード簡素化

VideoSight は

- STATUS確認
- データ取得

のみで動作する。


---

# 13. v1.0 完了条件

以下が確認された。

- ボタン入力取得
- エンコーダ差分取得
- 外部イベント取得
- EVENT通知

以上により **VideoSight ↔ PeripheralMCU プロトコル v1.0** を確定とする。

