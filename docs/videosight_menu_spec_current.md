# VideoSight メニュー画面仕様（現段階）

この文書は、ChatGPT会話ログからメニュー/UIに関する情報だけを抽出して整理したもの。  
実装前提のドラフト仕様であり、プロトタイプ検証で更新する。

## 1. 仕様ステータス

## 1.1 確定に近い項目
- 操作ボタンは `FRONT / BACK / UP / DOWN` の4系統で設計する。
- `FRONT` は実行系（短押し/長押しを区別）、`BACK` は戻る/中止系。
- 長押し判定は「押下時ではなくリリース時」に行う。
- 長押し成立時は短押しイベントを発生させない。
- メニューは射撃画面に対するオーバーレイ運用を基本にする。
- 録画中は `REC` 表示を常時出す。
- 文言の更新方針:
  - RecordingModeの `QUIT` は `STOP` へ変更
  - List/Editの `QUIT MENU` は `EXIT MENU` へ変更

## 1.2 準確定項目
- `BACK_HOLD` は優先度を高くし、値編集中でも確認なしで破棄してメニュー離脱。
- `FRONT` の短押し/長押し競合:
  - 押下中は何もしない
  - 解放時に短押し/長押し判定
  - 長押し成立時は `SNAP` を発生させない
- Recording中の `BRT` 変更は許可（録画処理とは独立）。

## 1.3 保留項目
- 最終メニュー項目ツリー（カテゴリ名/プロパティ名）
- 各プロパティの編集方式（数値/列挙/トグル）
- 画面象限ごとの表示固定位置（最終レイアウト）

## 2. 画面モード構成

- `MainMode`（射撃優先）
- `RecordingMode`（録画中）
- `ListMode`（項目選択）
- `EditMode`（値編集）

補助表示:
- `SNAP` トースト（短時間表示）
- `BRT` 値トースト（短時間表示）

## 3. ボタン機能（現段階）

## 3.1 MainMode
- `UP`: 輝度アップ
- `DOWN`: 輝度ダウン
- `FRONT(short)`: SNAP
- `FRONT(long)`: REC開始
- `BACK(short)`: MENU表示

## 3.2 RecordingMode
- `UP/DOWN`: 輝度変更
- `FRONT(short)`: 録画停止（STOP）
- `BACK(short)`: 録画停止（STOP）
- `Timeout`: 自動停止（例: 10秒）

## 3.3 ListMode
- `UP`: 前項目
- `DOWN`: 次項目
- `FRONT(short)`: 選択/下位へ
- `BACK(short)`: 1階層戻る
- `BACK(long)`: `EXIT MENU`（Mainへ戻る）

## 3.4 EditMode
- `UP`: 値増加 / 前候補
- `DOWN`: 値減少 / 次候補
- `FRONT(short)`: OK（確定）
- `BACK(short)`: CANCEL（破棄）
- `BACK(long)`: `EXIT MENU`（Mainへ戻る）

## 4. オーバーレイ表示ルール

## 4.1 MainMode（背景透過）
表示ガイド例:
```text
[▲▼]BRT
[F▶]SNAP
[F_HOLD▶]REC
[◀B]MENU
```

## 4.2 RecordingMode（背景透過）
表示ガイド例:
```text
[▲▼]BRT
[F▶]STOP

[◀B]STOP
●REC 07s
```

## 4.3 ListMode（背景不透過）
- 青背景/白文字を基本に、選択行は反転表示。
- 最上/中間/最下でガイドを切り替えて可動方向を示す。

表示記号:
- `[ ▼]` 最上段または最大値
- `[▲▼]` 中間
- `[▲ ]` 最下段または最小値

## 4.4 EditMode（背景不透過）
- 編集対象名 + 大きい値表示 + `CANCEL/OK` ガイド。
- 値表示部は視認性優先（強調表示）。

## 5. 画面遷移（現段階）

```text
Main
 ├─ FRONT(long) ─> Recording
 └─ BACK(short) ─> List(categories)

Recording
 ├─ FRONT(short) ─> Main
 ├─ BACK(short)  ─> Main
 └─ Timeout      ─> Main

List(categories)
 ├─ FRONT ─> List(properties) or Edit
 ├─ BACK  ─> 1階層戻る
 └─ BACK(long) ─> Main

Edit
 ├─ FRONT ─> List(properties) [確定]
 ├─ BACK  ─> List(properties) [破棄]
 └─ BACK(long) ─> Main
```

## 6. 表示テキスト運用方針

- ユーザーはモード名を意識しない前提で文言を作る。
- `TO MAIN` のような内部状態依存語は避ける。
- 動詞は操作結果を直接示す語を優先:
  - `STOP`
  - `EXIT MENU`
  - `CANCEL`
  - `OK`

## 7. 実装時の注意（暫定）

- 入力デバウンスと長押し時間を先に固定する。
- 画面遷移直後の短時間入力マスクを検討する。
- 複数同時押しは原則無効化し、競合時は何もしない。
- `SNAP/BRT/REC` の表示重なりは座標で衝突回避する。
