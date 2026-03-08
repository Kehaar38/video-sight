### ボタンシンボル
以下のシンボルが表示されている時、当該ボタンが有効になる。
`[◀B_HOLD]/[F_HOLD▶]`の表示が無いときは短押し/長押しの区別はない。
```
[◀B]        BACK
[◀B_HOLD]   BACK長押し
[F▶]        FRONT
[F_HOLD▶]   FRONT長押し
[ ▼]        UP
[▲▼]        UP/DOWN
[▲ ]        DOWN
```
項目の最上/最下、最大/最小で表示を切り替えてそれ以上進まないことを示す。
```
[ ▼]  項目の一番上/最大値
[▲▼]  項目の中間/中間値
[▲ ]  項目の一番下/最小値
```


### オーバーレイ
#### MainMode
背景透過/白文字（射撃優先仕様）
```
[▲▼]BRT
[F▶]SNAP
[F_HOLD▶]REC
[◀B]MENU
```

輝度最大
```
[ ▼]BRT
[F▶]SNAP
[F_HOLD▶]REC
[◀B]MENU
```

スナップショット時
`BRT`と`●REC`に被らない位置
RecordingMode時も同様
```
[ ▼]BRT
[F▶]SNAP
[F_HOLD▶]REC
[◀B]MENU

●SNAP    <-水色背景/白文字, 1秒間表示
```

輝度変更時
`BRT`と`●SNAP`に被らない位置
RecordingMode時も同様
```
[ ▼]BRT
[F▶]SNAP
[F_HOLD▶]REC
[◀B]MENU


BRT 10    <-白背景/青文字, 1秒間表示
```

#### RecordingMode
背景透過/白文字（射撃優先仕様）
mainオーバーレイからレイアウトを継承して`[F_HOLD▶]REC`の行を空ける
```
[▲▼]BRT
[F▶]STOP

[◀B]STOP
●REC 07s    <-赤背景/白文字, カウントダウン表示
```

同時にスナップショットと輝度変更
`[F_HOLD▶]REC`押下時間 > `●SNAP`表示時間 の場合は実現しないが定義しておく
```
[▲▼]BRT
[F▶]STOP

[◀B]STOP
●REC 07s
●SNAP
BRT 08
```

#### ListMode
青背景/白文字
一番上の項目を選択
```
«Categories»
[ ▼]Display    <-白背景/青文字, 以下同様
    Ballistic
    Calibration
    Color
[◀B]BACK  SELECT[F▶]
[◀B_HOLD]EXIT MENU
```
2番目の項目を選択
```
«Categories»
    Display
[▲▼]Ballistic
    Calibration
    Color
[◀B]BACK  SELECT[F▶]
[◀B_HOLD]EXIT MENU
```
3番目の項目を選択　ガイドは動かずリストがスクロール
```
«Categories»
    Ballistic
[▲▼]Calibration
    Color
    Power supply
[◀B]BACK  SELECT[F▶]
[◀B_HOLD]EXIT MENU
```
4番目の項目を選択　リストが一番下まで表示されたのでガイドが動く
```
«Categories»
    Ballistic
    Calibration
[▲▼]Color
    Power supply
[◀B]BACK  SELECT[F▶]
[◀B_HOLD]EXIT MENU
```
一番下の項目を選択
```
«Categories»
    Ballistic
    Calibration
    Color
[▲ ]Power supply
[◀B]BACK  SELECT[F▶]
[◀B_HOLD]EXIT MENU
```

#### EditMode
青背景/白文字
```
«Vertical offset»

 [▲▼] ＋１００ mm    <-数字のみ白背景/青文字, 大フォント

[◀B]CANCEL    OK[F▶]
[◀B_HOLD]EXIT MENU
```

最小値
```
«Vertical offset»

 [▲ ] －５００ mm    <-数字のみ白背景/青文字, 大フォント

[◀B]CANCEL    OK[F▶]
[◀B_HOLD]EXIT MENU
```


### 画面遷移図
```mermaid
classDiagram
class MainMode{
  メインモード
  電源投入後このモードへ遷移する
  最もオーバーレイが少なく射撃に適したモード
  
  -UP() 輝度アップ
  -DOWN() 輝度ダウン
  -FRONT() スナップショット
  -FRONT_hold() 録画開始
  -BACK() メニュー表示
}

class RecordingMode{
  録画中モード
  自動的に録画開始（連続写真）
  録画は10秒で自動終了する
  
  -UP() 輝度アップ
  -DOWN() 輝度ダウン
  -FRONT() 録画終了
  -BACK() 録画終了
  -Timeout() 録画終了
}

class snap{
  ガイドの下に"●SNAP"を表示する
  1秒後に消える
  
  -Timeout() 表示終了
}

class brt{
  ガイドの下に"BRT 輝度"を表示する
  1秒後に消える
  
  -Timeout() 表示終了
}

class ListMode_categories{
  メニューモード カテゴリー選択画面
  ListModeクラスは共通
  深さは要求に応じて柔軟に対応
  
  -UP() 項目前
  -DOWN() 項目次
  -FRONT() 項目選択
  -BACK() 前の画面へ戻る
  -BACK_hold() メインモードへ戻る
}

class ListMode_properties{
  メニューモード プロパティ選択画面
  ListModeクラスは共通
  深さは要求に応じて柔軟に対応
  
  -UP() 項目前
  -DOWN() 項目次
  -FRONT() 項目選択
  -BACK() 前の画面へ戻る
  -BACK_hold() メインモードへ戻る
}

class EditMode{
  プロパティ編集画面
  
  -UP() 数値アップ/リスト前
  -DOWN() 数値ダウン/リスト次
  -FRONT() 編集確定
  -BACK() 編集キャンセル
  -BACK_hold() メインモードへ戻る
}

MainMode --> RecordingMode: FRONT_hold
MainMode --> ListMode_categories: BACK
MainMode --> snap: FRONT
MainMode --> brt: UP/DOWN

snap --> MainMode: Timeout
snap --> RecordingMode: Timeout

brt --> MainMode: Timeout
brt --> RecordingMode: Timeout

ListMode_categories --> ListMode_properties: FRONT
ListMode_categories --> MainMode: BACK
ListMode_categories --> MainMode: BACK_hold

ListMode_properties --> EditMode: FRONT
ListMode_properties --> ListMode_categories: BACK
ListMode_properties --> MainMode: BACK_hold

EditMode --> ListMode_properties: FRONT
EditMode --> ListMode_properties: BACK
EditMode --> MainMode: BACK_hold

RecordingMode --> MainMode: FRONT
RecordingMode --> MainMode: BACK
RecordingMode --> MainMode: Timeout
RecordingMode --> brt: UP/DOWN

```