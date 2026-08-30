# 9. 固定小数点数

> **この章で分かること**
>
> - なぜ DS で `float` を避けるのかを説明できる
> - 12bit 固定小数点で座標と速度を扱える
> - libnds の `f32` と `sinLerp` を使える

---

## 9.1 なぜ `float` を使わないのか

BlocksDS のテンプレートのビルドフラグを見てください。

```makefile
ARCH := -mthumb -mcpu=arm946e-s+nofp
```

**`nofp` = no floating point。DS の ARM9 には FPU（浮動小数点演算ユニット）がありません。**

したがって `float a * b` と書くと、CPU 命令 1 個には翻訳されず、
`__aeabi_fmul` のような**ソフトウェアのライブラリ関数呼び出し**になります。

| 演算 | だいたいのコスト |
| --- | --- |
| `int` の加算 | 1 サイクル |
| `int` の乗算 | 数サイクル |
| `float` の加算・乗算 | **数十サイクル**（関数呼び出し） |
| `float` の除算 | **さらに遅い** |

DS の ARM9 は 67MHz です。60fps を維持するには 1 フレームあたり約 110 万サイクルしかありません。
敵 30 体 × 座標 2 軸 × 数回の演算を `float` でやるだけで、
無視できない割合を食い潰します。

そこで使うのが **固定小数点数** です。

---

## 9.2 固定小数点の考え方

アイデアは驚くほど単純です。

> **整数を、あらかじめ決めた倍率（2 のべき乗）で掛けたものとして扱う。**

たとえば「1.0 を 4096 (= 2^12) として表す」と決めます。

| 表したい値 | 内部の整数値 |
| --- | --- |
| 1.0 | 4096 |
| 2.0 | 8192 |
| 0.5 | 2048 |
| 0.25 | 1024 |
| 1.5 | 6144 |
| -1.0 | -4096 |

これを **12bit 固定小数点** や **Q12 形式** と呼びます。
32bit の `int` の中で、**上位 20bit が整数部、下位 12bit が小数部**になっているイメージです。

```
  31                        12 11              0
 ┌────────────────────────────┬─────────────────┐
 │      整数部 (20bit)         │  小数部 (12bit)  │
 └────────────────────────────┴─────────────────┘
```

- 表せる整数の範囲: 約 ±524,288（DS の画面は 256×192 なので余裕）
- 表せる最小の刻み: 1/4096 ≈ 0.00024

**すべての演算が整数演算のままなので、CPU にとっては単なる `int` です。**

---

## 9.3 変換

```c
#define FP_SHIFT 12
#define FP_ONE   (1 << FP_SHIFT)       // 4096

// int → 固定小数点
static inline int intToFp(int v)   { return v << FP_SHIFT; }

// 固定小数点 → int（小数部を切り捨て）
static inline int fpToInt(int v)   { return v >> FP_SHIFT; }

// float → 固定小数点（コンパイル時の定数計算に使う。実行時には使わない）
#define FLOAT_TO_FP(f) ((int)((f) * FP_ONE))
```

使用例:

```c
int x  = intToFp(128);         // 座標 128.0 → 524288
int vx = FLOAT_TO_FP(1.5);     // 速度 1.5 → 6144

x += vx;                       // 加算はそのまま！
                               // 129.5 を表す 530432 になる

int screenX = fpToInt(x);      // 描画時に整数に戻す → 129
```

---

## 9.4 四則演算のルール

### 加算・減算 — そのまま

```c
int a = intToFp(3);      // 12288
int b = intToFp(2);      //  8192
int c = a + b;           // 20480 = intToFp(5)  ✅ 正しい
```

倍率が同じもの同士なので、何もしなくて合います。

### 乗算 — 掛けた後に右シフト

```c
int a = FLOAT_TO_FP(2.0);   //  8192
int b = FLOAT_TO_FP(3.0);   // 12288

int wrong = a * b;          // 100,663,296。これは 24576.0 で 12288 倍ずれている
int right = (a * b) >> FP_SHIFT;   // 24576 = intToFp(6)  ✅
```

理由: `(x * 4096) * (y * 4096) = x*y * 4096²` なので、1 回分の 4096 で割り戻す必要があります。

**注意: 途中でオーバーフローします。**

```c
int a = intToFp(1000);      // 4,096,000
int b = intToFp(1000);      // 4,096,000
int c = (a * b) >> 12;      // a * b = 1.67×10^13。int(32bit) に収まらない！
```

大きな値を掛けるときは 64bit を経由します。

```c
static inline int fpMul(int a, int b)
{
    return (int)(((s64)a * b) >> FP_SHIFT);
}
```

64bit 乗算は 32bit よりは遅いですが、`float` よりはるかに速いです。
値が小さいと分かっている場所（速度 × 時間など）では `(a * b) >> 12` で十分です。

### 除算 — 割る前に左シフト（発展）

```c
static inline int fpDiv(int a, int b)
{
    return (int)(((s64)a << FP_SHIFT) / b);
}
```

ARM9 には除算命令が無いので重いです。libnds の `divf32()` は
DS のハードウェア除算器を使うので、こちらを使ってください。

### 定数で掛ける・割る場合はシフトだけでよい

```c
x = x / 2;        // 遅い
x = x >> 1;       // 速い（x が符号なし、または負にならないと分かっている場合）
```

**重力や摩擦の係数を 2 のべき乗に設計しておく**と、シフトだけで済んで速いです。
これはゲーム開発では非常によく使われるテクニックです。

```c
vy += GRAVITY;          // 落下
vx -= vx >> 5;          // 摩擦（速度の 1/32 を毎フレーム引く）
```

---

## 9.5 libnds の固定小数点型

libnds は `nds/arm9/math.h` と `nds/arm9/videoGL.h` で型とマクロを用意しています。

| 型 | 形式 | 1.0 の値 | 用途 |
| --- | --- | --- | --- |
| `f32` | 1.19.12（符号 1 + 整数 19 + 小数 12） | 4096 | 汎用 |
| `v16` | 1.3.12 | 4096 | 3D の頂点座標 |
| `t16` | 1.11.4 | 16 | テクスチャ座標 |

変換マクロ（`nds/arm9/math.h` の実物）:

```c
#define inttof32(n)    ((n) * (1 << 12))        // int → f32
#define f32toint(n)    ((n) / (1 << 12))        // f32 → int
#define floattof32(n)  ((int)((n) * (1 << 12))) // float → f32
#define f32tofloat(n)  (((float)(n)) / (float)(1 << 12))
```

つまり **libnds の標準も 12bit 固定小数点**です。自作の `FP_SHIFT 12` と同じなので、
そのまま混ぜて使えます。

---

## 9.6 三角関数

`sinf()` / `cosf()` は浮動小数点でとても遅いので、libnds はテーブル引きの関数を用意しています
（`nds/arm9/trig_lut.h`）。

```c
s16 sinLerp(s16 angle);     // sin。戻り値は 1.3.12 形式（4096 = 1.0）
s16 cosLerp(s16 angle);
```

### 角度の単位に注意

**libnds の角度は度でもラジアンでもありません。一周が 32768（= 2^15）です。**

```c
#define DEGREES_IN_CIRCLE       (1 << 15)                        // 32768
#define degreesToAngle(degrees) ((degrees) * DEGREES_IN_CIRCLE / 360)
#define angleToDegrees(angle)   ((angle) * 360 / DEGREES_IN_CIRCLE)
```

一周を 2 のべき乗にしておくと、**角度のラップアラウンドがビット演算で済む**という利点があります。

```c
angle = (angle + 100) & (DEGREES_IN_CIRCLE - 1);   // 0〜32767 に自動で収まる
```

### 使用例: 角度と速度から移動量を出す

```c
// 角度 angle の方向に speed（f32）で進む
s16 angle = degreesToAngle(45);
int speed = inttof32(2);                    // 2.0

int vx = mulf32(speed, cosLerp(angle));     // cosLerp は 4096 = 1.0
int vy = mulf32(speed, sinLerp(angle));

x += vx;
y += vy;

// 描画時
oamSetXY(&oamMain, 0, f32toint(x), f32toint(y));
```

---

## 9.7 実践: 弾を飛ばす

固定小数点の典型的な使い方をまとめます。

```c
#include <nds.h>

#define FP 12                       // 12bit 固定小数点
#define TO_FP(n)  ((n) << FP)
#define TO_INT(n) ((n) >> FP)

typedef struct {
    bool active;
    int  x, y;        // 位置（固定小数点）
    int  vx, vy;      // 速度（固定小数点）
    int  gfx;
} Bullet;

#define GRAVITY   (TO_FP(1) / 16)   // 0.0625 px/frame^2

void bulletUpdate(Bullet *b)
{
    if (!b->active) return;

    b->vy += GRAVITY;               // 重力
    b->x  += b->vx;                 // 加算はそのまま
    b->y  += b->vy;

    if (TO_INT(b->y) > 192) b->active = false;
}

void bulletDraw(const Bullet *b, int oamId)
{
    if (!b->active) {
        oamSetHidden(&oamMain, oamId, true);
        return;
    }
    // 描画するときだけ整数に戻す
    oamSetXY(&oamMain, oamId, TO_INT(b->x), TO_INT(b->y));
}
```

**設計の要点:**

1. **内部の状態は全部固定小数点で持つ**（1 ピクセル未満の動きが表現できる）
2. **描画の瞬間だけ整数に落とす**
3. 加算・減算はそのまま、乗算だけ `>> 12` を忘れない
4. 係数はできるだけ 2 のべき乗にしてシフトで済ませる

「1 フレームに 0.5px 動く」が表現できるのが固定小数点の大きな利点です。
整数だけで座標を持つと、遅い移動やなめらかな加速が作れません。

---

## 確認問題

**Q1.** 12bit 固定小数点で、`1.25` を表す整数値はいくつですか。

<details><summary>解答</summary>

`1.25 × 4096 = 5120`

コードでは `FLOAT_TO_FP(1.25)` あるいは `(1 << 12) + (1 << 10)` と書けます。
</details>

**Q2.** 次のコードは正しくありません。何が間違っていますか。

```c
int a = intToFp(3);      // 3.0
int b = intToFp(4);      // 4.0
int c = a * b;           // 12.0 のつもり
```

<details><summary>解答</summary>

固定小数点の乗算は、掛けた後に `>> 12` して倍率を 1 回分戻す必要があります。

```
a = 3 * 4096 = 12288
b = 4 * 4096 = 16384
a * b = 201,326,592 = 12 * 4096 * 4096
```

つまり結果は 4096 倍ずれています。

```c
int c = (a * b) >> 12;              // 49152 = intToFp(12)  ✅
int c = fpMul(a, b);                // オーバーフロー対策込み（64bit 経由）
int c = mulf32(a, b);               // libnds の関数
```

</details>

**Q3.** DS でこの計算を毎フレーム 100 回行うのは避けるべきです。なぜですか。書き換えてください。

```c
float angle = 0.0f;
float x = cx + cosf(angle) * radius;
float y = cy + sinf(angle) * radius;
```

<details><summary>解答</summary>

**理由:** ARM946E-S に FPU が無いため、`cosf` / `sinf` / `float` の乗算はすべて
ソフトウェアエミュレーション（ライブラリ関数呼び出し）になり、
整数演算の数十倍のコストがかかります。67MHz の CPU では 60fps を維持できません。

**書き換え:** libnds のテーブル引き三角関数と固定小数点を使います。

```c
s16 angle = 0;                                  // 一周 = 32768
int radius = inttof32(40);                      // f32

int x = cx + mulf32(cosLerp(angle), radius);    // cosLerp は 4096 = 1.0
int y = cy + mulf32(sinLerp(angle), radius);

// 描画時
oamSetXY(&oamMain, 0, f32toint(x), f32toint(y));

// 角度を進める（ラップアラウンドはビット演算で）
angle = (angle + 200) & (DEGREES_IN_CIRCLE - 1);
```

`sinLerp` / `cosLerp` は事前計算されたテーブルの線形補間なので、
数命令で結果が得られます。
</details>

---

[← 前章: 8. ビット演算](08-bitops.md) | [目次](../README.md) | [次章: 10. ゲームコードの型 →](10-game-patterns.md)
