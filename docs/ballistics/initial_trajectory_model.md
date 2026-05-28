# 初期弾道予測モデル

## 目的

VIDEO SIGHTの初期弾道予測モデルは、実射精度を最初から追い込むための物理シミュレーターではなく、次の機能を先に成立させるための表示用モデルである。

```text
- 固定値から弾道点を計算できる
- LCDへ弾道点をプロットできる
- IMU由来のpitch/rollで表示が変化する
- ハードウェア、描画、IMU、UIの開発を進められる
```

この段階では、風、地面、yaw、射撃中の姿勢変化、実射補正、回転減衰、揚力/抗力係数の変化は扱わない。

## モデルの考え方

このモデルでは、世界座標を明示的に持たない。弾道は、照準器/カメラに固定された `f/r/u` 座標の中で計算する。

```text
f: forward / カメラ前方 / 銃口方向
r: right   / 画面右方向
u: up      / 画面上方向 / ホップ揚力の基準方向
```

弾道予測は「今の姿勢で撃った場合に、カメラ画面内で弾道点がどこへ出るか」を求めるものとする。

そのため、次のものは考慮しない。

```text
風向き:
  測れないため考慮しない。

地面に対するターゲット位置:
  ターゲットはf方向距離だけで扱う。

射撃中の姿勢変化:
  今時点の姿勢での予測表示のみ扱う。

方位/yawを含む空間位置:
  カメラから覗く世界だけを扱うため考慮しない。

地面:
  地形や設置物により一定の地面を仮定できないため、モデルには入れない。
```

## 初期固定値

初期段階では、弾・銃・空気・ホップ関連の値を固定する。

```text
BB重量: 0.20 g
BB直径: 6.0 mm
初速: 90 m/s
空気密度: 1.225 kg/m^3
抗力係数 C_D: 0.47
マグヌス係数 C_L: 0.038
最大予測距離: 40 m
距離ステップ: 2 m
プロット点数: 21点
ゼロイン距離: 30 m相当
サイト高: 仮値 0.05 m
```

計算用の値:

```text
m   = 0.0002 kg
rBB = 0.003 m
A   = π × rBB²
ρ   = 1.225 kg/m^3
Cd  = 0.47
Cl  = 0.038
g   = 9.80665 m/s^2
v0  = 90.0 m/s
```

断面積:

```text
A = π × 0.003²
  ≒ 2.8274e-5 m²
```

抗力・揚力用の定数:

```text
K_drag = 0.5 × ρ × A × Cd / m
K_lift = 0.5 × ρ × A × Cl / m
```

上記固定値では:

```text
K_drag ≒ 0.04070
K_lift ≒ 0.00329
```

## IMUから使う値

初期段階では、IMUから次の2値だけを使う。

```text
pitch = θ
roll  = φ
```

単位はラジアンとする。

```text
pitch θ > 0:
  銃口を上へ向ける。

roll φ > 0:
  銃を右へ倒す。
```

`yaw` は使わない。BNO055のyawは磁気環境の影響を受けやすく、VIDEO SIGHTの初期表示では必要ないため。

## 状態量

弾の位置と速度は `f/r/u` 成分で持つ。

```text
位置:
  p = (pf, pr, pu)

速度:
  v = (vf, vr, vu)
```

初期値:

```text
pf = 0
pr = 0
pu = 0

vf = v0
vr = 0
vu = 0
```

つまり、初速方向は常に `+f` とする。

## 重力

世界座標は持たないが、重力が現在の `f/r/u` 座標でどの方向に見えるかをpitch/rollから分解する。

```text
g_f = -g × sinθ
g_r =  g × cosθ × sinφ
g_u = -g × cosθ × cosφ
```

例:

```text
水平・ロールなし:
  pitch = 0°
  roll  = 0°

  g_f = 0
  g_r = 0
  g_u = -g

  → 画面下方向へ落ちる。
```

```text
真上に向ける:
  pitch = 90°

  g_f = -g
  g_r = 0
  g_u = 0

  → 前進方向に逆らう。
```

```text
水平・右90度ロール:
  pitch = 0°
  roll  = 90°

  g_f = 0
  g_r = +g
  g_u = 0

  → 画面右方向へ落ちる。
```

## 抗力

抗力は速度ベクトルと逆向きに計算する。

速度の大きさ:

```text
speed = sqrt(vf² + vr² + vu²)
```

抗力:

```text
a_drag = -K_drag × speed × v
```

成分:

```text
a_drag_f = -K_drag × speed × vf
a_drag_r = -K_drag × speed × vr
a_drag_u = -K_drag × speed × vu
```

## マグヌス揚力

ホップ揚力の基準方向は `+u` とする。ただし、揚力は速度に対して垂直に働くものとして、`+u` から速度方向成分を取り除く。

速度単位ベクトル:

```text
v_hat = v / speed
```

基準上方向:

```text
u_axis = (0, 0, 1)
```

`u_axis` から速度方向成分を除去する。

```text
lift_dir = u_axis - dot(u_axis, v_hat) × v_hat
lift_dir = normalize(lift_dir)
```

揚力:

```text
a_lift = K_lift × speed² × lift_dir
```

成分:

```text
a_lift_f = K_lift × speed² × lift_dir_f
a_lift_r = K_lift × speed² × lift_dir_r
a_lift_u = K_lift × speed² × lift_dir_u
```

これにより、ホップ揚力は画面上方向を基準にしつつ、弾の実際の速度方向に対して垂直になる。

## 合計加速度

```text
a_f = a_drag_f + a_lift_f + g_f
a_r = a_drag_r + a_lift_r + g_r
a_u = a_drag_u + a_lift_u + g_u
```

## 数値積分

時間刻み `dt` で速度と位置を更新する。

```text
vf += a_f × dt
vr += a_r × dt
vu += a_u × dt

pf += vf × dt
pr += vr × dt
pu += vu × dt
```

`dt` はまず次を使う。

```text
dt = 0.001 s
```

必要に応じて、見た目や安定性確認用に次へ小さくしてよい。

```text
dt = 0.0005 s
```

## nメートル先の点を取る

距離は `f` 方向距離だけを扱う。

```text
pf >= target_distance_m
```

になったところを、nメートル先の点とする。

より滑らかにするため、前ステップとの線形補間を行う。

```text
alpha = (target_distance_m - pf_prev) / (pf - pf_prev)

pr_n = pr_prev + alpha × (pr - pr_prev)
pu_n = pu_prev + alpha × (pu - pu_prev)
```

画面上の変位:

```text
screen_x_m = pr_n
screen_y_m = pu_n
```

意味:

```text
screen_x_m:
  照準中心から右方向へのズレ[m]

screen_y_m:
  照準中心から上方向へのズレ[m]
```

## LCD座標への変換

最初は単純なスケールでよい。

```text
px = center_x + screen_x_m × scale
py = center_y - screen_y_m × scale
```

LCD座標は下方向が正なので、`screen_y_m` は符号を反転して使う。

カメラ視野角に合わせる段階では、角度ベースにする。

```text
angle_x = atan2(screen_x_m, target_distance_m)
angle_y = atan2(screen_y_m, target_distance_m)

px = center_x + fx_px × tan(angle_x)
py = center_y - fy_px × tan(angle_y)
```

初期の見た目確認では、単純スケールで十分とする。

## C風擬似コード

```c
// f/r/u frame
// p = (pf, pr, pu)
// v = (vf, vr, vu)

float theta = pitch_rad;
float phi   = roll_rad;

// gravity in f/r/u frame
float gf = -g * sinf(theta);
float gr =  g * cosf(theta) * sinf(phi);
float gu = -g * cosf(theta) * cosf(phi);

float speed = sqrtf(vf*vf + vr*vr + vu*vu);

// drag
float adf = -K_drag * speed * vf;
float adr = -K_drag * speed * vr;
float adu = -K_drag * speed * vu;

// lift direction from +u, perpendicular to velocity
float vhat_f = vf / speed;
float vhat_r = vr / speed;
float vhat_u = vu / speed;

// u_axis = (0, 0, 1)
float dot_u_v = vhat_u;

float lift_f = 0.0f - dot_u_v * vhat_f;
float lift_r = 0.0f - dot_u_v * vhat_r;
float lift_u = 1.0f - dot_u_v * vhat_u;

float lift_len = sqrtf(lift_f*lift_f + lift_r*lift_r + lift_u*lift_u);

if (lift_len > 1e-6f) {
    lift_f /= lift_len;
    lift_r /= lift_len;
    lift_u /= lift_len;
} else {
    lift_f = 0.0f;
    lift_r = 0.0f;
    lift_u = 1.0f;
}

float lift_accel = K_lift * speed * speed;

float alf = lift_accel * lift_f;
float alr = lift_accel * lift_r;
float alu = lift_accel * lift_u;

// total acceleration
float af = adf + alf + gf;
float ar = adr + alr + gr;
float au = adu + alu + gu;

// integrate
vf += af * dt;
vr += ar * dt;
vu += au * dt;

pf += vf * dt;
pr += vr * dt;
pu += vu * dt;
```

nメートル先を取る場合:

```c
if (pf >= target_distance_m) {
    float alpha = (target_distance_m - pf_prev) / (pf - pf_prev);

    float pr_n = pr_prev + alpha * (pr - pr_prev);
    float pu_n = pu_prev + alpha * (pu - pu_prev);

    screen_x_m = pr_n;
    screen_y_m = pu_n;
}
```

## プロット仕様

初期プロットは次を基本とする。

```text
距離: 0〜40m
間隔: 2m
点数: 21点
```

```text
0, 2, 4, ..., 40
```

表示上は、10mごとに強調してもよい。

```text
2mごと: 小さい点
10mごと: 大きい点または色違い
ゼロイン距離: 強調表示
```

## 将来的な課題

初期モデルでは次を固定または未対応とする。

```text
C_Dの速度/スピン依存
C_Lの速度/スピン依存
BB弾の回転数
回転減衰
遠方で強くホップアップする挙動
弾種切替
実射補正
ゼロイン実測調整
サイト高実測調整
```

遠方でフラットに飛んだ後に強くホップアップするような実弾道を再現するには、少なくとも次が必要になる可能性がある。

```text
S = rBB × omega / speed
C_L = f(S)
omegaの減衰
必要なら C_D = f(S, speed)
```

ただし、これは初期弾道予測モデルの完了条件には含めない。

## 初期モデルの完了条件

```text
- 固定値から0〜40m/2m刻みの弾道点を計算できる。
- LCDへ予測点を描画できる。
- pitchで重力方向のf/u成分が変化する。
- rollで重力方向のr/u成分が変化する。
- 揚力は+u基準で速度に垂直に働く。
- 抗力は速度と逆向きに働く。
- yaw、風、地面、射撃中の姿勢変化は扱わない。
```
