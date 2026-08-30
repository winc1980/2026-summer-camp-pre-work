# 3. 関数まわり

> **この章で分かること**
>
> - C の引数は全部コピーで渡される
> - 複数の値を返す方法
> - `static` の意味

`if` / `for` / `while` は他の言語とほぼ同じなので説明しません。
**違うところだけ**拾っていきます。

---

## 3.1 制御構文で気をつけること

### 真偽値は整数

**0 が偽、0 以外が真**です。

```c
if (hp)    { }    // hp != 0 と同じ
if (!ptr)  { }    // ptr == NULL と同じ
```

### `=` と `==` の取り違え

```c
if (hp = 0) { ... }    // 代入してしまっている。エラーにならない
```

`-Wall` を付けていれば警告が出ます。**警告を必ず読んでください。**

### `switch` は `break` を忘れると流れ落ちる

```c
switch (state) {
case STATE_TITLE:
    updateTitle();
    break;              // ← 忘れると次の case に落ちる

case STATE_PAUSE:
case STATE_MENU:        // 意図的にまとめる書き方
    updateMenu();
    break;
}
```

`case` の中で変数を宣言したいときは `{ }` で囲みます。`switch` に使えるのは整数だけです。

---

## 3.2 関数は呼ぶ前に宣言が必要

```c
int add(int a, int b);      // 宣言（プロトタイプ）

int add(int a, int b)       // 定義（実体）
{
    return a + b;
}
```

コンパイラはファイルを上から読むので、下で定義した関数を上から呼ぶには
先に宣言が要ります。だから宣言をヘッダに書いて `#include` します（1 章）。

宣言なしで呼ぶとこの警告が出ます。

```
warning: implicit declaration of function 'foo'
```

**必ず直してください。** ヘッダの `#include` 漏れか、関数名のタイプミスです。

> 引数が無い関数は `void f(void)` と書きます。`void f()` は歴史的に意味が違うためです。

---

## 3.3 【重要】引数は全部コピーされる

C の引数渡しは**例外なくコピー**です。

```c
void addTen(int x)
{
    x += 10;          // コピーを書き換えているだけ
}

int score = 5;
addTen(score);
printf("%d\n", score);   // 5。変わらない
```

呼び出し元の変数を書き換えたければ、**アドレスを渡します**。

```c
void addTen(int *x)
{
    *x += 10;         // x が指す先を書き換える
}

int score = 5;
addTen(&score);
printf("%d\n", score);   // 15
```

これが [4 章](04-pointers.md)（ポインタ）の本題です。
libnds の API もこの形をよく使います。

```c
touchPosition touch;
touchRead(&touch);           // touch の中身が書き込まれる
printf("%d,%d\n", touch.px, touch.py);
```

構造体も同じくコピーされます。大きな構造体をそのまま渡すと**丸ごとコピー**されるので、
DS では基本ポインタで渡します（6 章）。

**配列だけは例外に見えます。** 配列を渡すと自動でポインタに化けるからです（5 章）。

---

## 3.4 複数の値を返したいとき

C の関数は値を 1 つしか返せません。定番は 2 つです。

### (a) ポインタ引数で受け取る（libnds もこれ）

```c
void getCenter(int *x, int *y)
{
    *x = 128;
    *y = 96;
}

int cx, cy;
getCenter(&cx, &cy);
```

### (b) 構造体を返す

```c
typedef struct { int x, y; } Point;

Point getCenter(void)
{
    Point p = { 128, 96 };
    return p;
}
```

### エラーは戻り値で返す

C には例外がありません。**エラーは必ず戻り値で伝え、呼び出し側がチェックします。**

```c
// 成功で 0、失敗で負の値
int loadTexture(const char *path, u16 **outData);

u16 *tex;
if (loadTexture("player.bin", &tex) != 0) {
    printf("load failed\n");
    return;
}
```

`malloc` の戻り値を見ない、が事故の定番です。

---

## 3.5 `static` — C における private

C には名前空間がありません。何も付けずに関数を作ると、
**プログラム全体で名前が衝突する**可能性があります。

```c
/* enemy.c */
static int sEnemyCount = 0;         // このファイルからしか見えない変数

static void spawnOne(int x, int y)  // このファイルからしか呼べない関数
{
    ...
}

void enemyUpdateAll(void)           // 外から呼べる（ヘッダに宣言を書く）
{
    ...
}
```

> **原則: ヘッダに宣言を書かない関数・変数には `static` を付ける。**

### 関数の中の `static` は意味が違う

**関数を抜けても値が残る変数**になります。

```c
int nextId(void)
{
    static int id = 0;    // 初期化は最初の 1 回だけ
    return id++;
}
// 0, 1, 2, ... と返る
```

DS では、**大きなバッファをスタックに置けない**ので（7 章）、
これを回避するテクニックとしてもよく使います。

```c
void draw(void)
{
    static u16 buffer[4096];   // スタックではなく静的領域に置かれる
}
```

---

## 3.6 `static inline`（発展）

小さい関数をヘッダに書きたいときの形です。

```c
/* util.h */
static inline int clampInt(int v, int lo, int hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}
```

マクロと違って**型チェックが効き、引数が 1 回しか評価されない**ので、
マクロより常にこちらが優れています。

---

## 確認問題

**Q1.** `score` が変わらないのはなぜですか。直してください。

```c
void reset(int s) { s = 0; }

int score = 100;
reset(score);
```

<details><summary>解答</summary>

C の引数はコピーなので、`reset` は `score` のコピーを 0 にしているだけです。

```c
void reset(int *s) { *s = 0; }

reset(&score);      // アドレスを渡す
```

</details>

**Q2.** `static` の 2 つの意味を説明してください。

<details><summary>解答</summary>

1. **グローバルの位置に書いた場合** — そのファイルの外から見えなくなる。
   C における `private`。名前の衝突を防ぐために積極的に付ける。

2. **関数の中に書いた場合** — 関数を抜けても値が残る。初期化は最初の 1 回だけ。
   実体はスタックではなく静的領域に置かれる。

</details>

**Q3.** `warning: implicit declaration of function 'oamSet'` が出ました。何をすべきですか。

<details><summary>解答</summary>

`oamSet` のプロトタイプ宣言が見えていません。必要なヘッダ（`#include <nds.h>`）を追加します。
自作関数なら、そのヘッダを include するか、ヘッダに宣言を書き足します。
関数名のタイプミスの可能性もあるので綴りも確認してください。

放置すると、コンパイラが勝手に型を仮定して先に進み、実行時に静かに壊れます。
</details>

---

[← 2. 型と数値](02-types.md) | [目次](../README.md) | [次章: 4. ポインタ →](04-pointers.md)
