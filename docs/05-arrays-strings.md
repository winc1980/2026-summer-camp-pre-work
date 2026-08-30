# 5. 配列と文字列

> **この章で分かること**
>
> - 配列がポインタに「減衰」する仕組みを理解する
> - 境界チェックが無い世界での配列の扱い方を身につける
> - C の文字列（`'\0'` 終端）を安全に扱える

---

## 5.1 配列の基本

```c
int scores[5];                      // 要素数 5。中身は未初期化（ゴミ）
int scores[5] = {10, 20, 30, 40, 50};
int scores[5] = {10, 20};           // 残りは 0 で埋まる
int scores[]  = {10, 20, 30};       // 要素数は 3 と推論される
int scores[5] = {0};                // 全部 0（定番のイディオム）

scores[0] = 1;                      // 添字は 0 始まり
```

要素数は**コンパイル時に決まる定数**でなければなりません。

```c
#define MAX_ENEMIES 32
Enemy enemies[MAX_ENEMIES];         // OK

int n = getEnemyCount();
Enemy enemies[n];                   // VLA（可変長配列）。DS では避ける
```

VLA はスタック上に動的サイズの領域を取るため、
**スタックの小さい DS では簡単に破綻します**（7 章）。使わないでください。

### 要素数の求め方

```c
int arr[10];
size_t n = sizeof(arr) / sizeof(arr[0]);   // 10
```

マクロにしておくと便利です。

```c
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
```

**ただしこれは「本物の配列」に対してしか動きません。** 理由は次節。

---

## 5.2 【最重要】配列はポインタに「減衰」する

C では、配列の名前を式の中で使うと、
**ほとんどの場面で「先頭要素へのポインタ」に自動変換されます**。
これを **配列からポインタへの減衰（array-to-pointer decay）** と言います。

```c
int arr[5] = {10, 20, 30, 40, 50};
int *p = arr;             // arr は &arr[0] に化ける。キャスト不要

printf("%d\n", arr[2]);   // 30
printf("%d\n", p[2]);     // 30   ポインタにも [] が使える
printf("%d\n", *(arr+2)); // 30
printf("%d\n", *(p+2));   // 30   全部同じ意味
```

つまり **`a[i]` は `*(a + i)` の糖衣構文**です。
（余談ですが、これは可換なので `2[arr]` とも書けます。書かないでください）

### 減衰しない例外

- `sizeof(arr)` — 配列全体のバイト数を返す
- `&arr` — 「配列全体へのポインタ」になる（`&arr[0]` とは型が違う）

この 2 つ以外は基本的に減衰すると思ってください。

### 関数に渡すと長さが失われる

**ここが最重要ポイントです。**

```c
void printAll(int arr[])         // 実は int *arr と完全に同じ
{
    size_t n = sizeof(arr) / sizeof(arr[0]);
    // sizeof(arr) は「ポインタのサイズ」= 4
    // sizeof(arr[0]) は 4
    // → n は常に 1 になる！バグ
}
```

引数の `int arr[]` という書き方は、コンパイラによって `int *arr` に読み替えられます。
`int arr[10]` と書いても同じで、**10 という情報は捨てられます**。

したがって C では、**長さを必ず一緒に渡します**。

```c
void printAll(const int *arr, size_t n)
{
    for (size_t i = 0; i < n; i++) {
        printf("%d ", arr[i]);
    }
}

int scores[5] = {1,2,3,4,5};
printAll(scores, ARRAY_SIZE(scores));    // 呼び出し側では sizeof が効く
```

**「配列と長さはセットで持ち回る」が C の作法です。**
構造体にまとめてしまうのも良い手です。

```c
typedef struct {
    int   *data;
    size_t count;
} IntArray;
```

---

## 5.3 境界チェックは無い

```c
int arr[5];
arr[10] = 99;      // コンパイルも通るし、実行時エラーも出ない
arr[-1] = 99;      // これも通る
```

**何が起きるか:** `arr` の隣にあるメモリが書き換わります。
そこが別の変数なら、その変数の値が謎に変わります。
関数の戻りアドレスならプログラムが暴走します。

これは C で最も多いバグの原因であり、そして**症状が原因から離れた場所に出ます**。
「敵の配列をいじったら BGM が止まった」みたいなことが本当に起きます。

対策:

```c
// 1. ループ条件を機械的に書く
for (int i = 0; i < MAX_ENEMIES; i++) { ... }      // <= にしない

// 2. マジックナンバーを配列サイズに直結させる
#define MAX_ENEMIES 32
Enemy enemies[MAX_ENEMIES];
for (int i = 0; i < MAX_ENEMIES; i++) { ... }

// 3. 添字を計算する場所ではガードを入れる
static inline int tileAt(int tx, int ty)
{
    if (tx < 0 || tx >= MAP_W || ty < 0 || ty >= MAP_H) return TILE_WALL;
    return map[ty][tx];
}
```

3 番目のパターンは、タイルマップを扱うゲームでは必須級です。
**画面外を参照するコードは必ず書かれます**。ならば最初からガードを入れておきます。

---

## 5.4 多次元配列

```c
int map[3][4] = {
    {0, 1, 2, 3},
    {4, 5, 6, 7},
    {8, 9, 10, 11},
};

printf("%d\n", map[1][2]);   // 6
```

**メモリ上は 1 列に並びます（行優先／row-major）。**

```
map[0][0] map[0][1] map[0][2] map[0][3] map[1][0] map[1][1] ...
    0         1         2         3         4         5
```

つまり `map[y][x]` は `map[y * 4 + x]` と同じ位置です。
DS のタイルマップやスプライトのピクセルデータは、この 1 次元表現で扱うことが多いです。

```c
#define MAP_W 32
#define MAP_H 24
u8 map[MAP_H][MAP_W];              // 2 次元で宣言してもよいし
u8 map[MAP_H * MAP_W];             // 1 次元で持って自分で計算してもよい

u8 tile = map[y * MAP_W + x];      // 後者の場合
```

> **（発展）** 行優先なので、**内側のループで x を回す**方がメモリアクセスが連続になり速いです。
> また、多次元配列を関数に渡すときは 2 番目以降の次元サイズを型に書く必要があります
> （`void f(u8 m[][MAP_W], int h)`）。面倒なので、DS のコードでは
> 1 次元配列 + 自前の添字計算がよく使われます。

---

## 5.5 文字列 — ただの `char` 配列

C に文字列型はありません。**文字列とは「`'\0'`（ヌル文字, 値 0）で終わる `char` の配列」**です。

```c
char s[] = "abc";
```

```
  s[0]  s[1]  s[2]  s[3]
 ┌─────┬─────┬─────┬─────┐
 │ 'a' │ 'b' │ 'c' │ '\0'│      ← サイズは 4 バイト（終端分を忘れずに）
 └─────┴─────┴─────┴─────┘
   97    98    99     0
```

**終端の `'\0'` を忘れると、`strlen` や `printf` がメモリの果てまで読み続けます。**
これも C の定番事故です。

### 配列とポインタ、2 つの宣言

```c
char  a[] = "hello";       // 6 バイトの配列。中身は書き換え可能
char *b   = "hello";       // 文字列リテラル（読み取り専用領域）へのポインタ

a[0] = 'H';                // OK
b[0] = 'H';                // 未定義動作！ ROM 上のデータを書き換えようとしている
```

DS では文字列リテラルは **ROM（カートリッジ）上**に置かれるので、書き換えは効きません。

**リテラルを指すポインタには必ず `const` を付けてください。**
そうすればコンパイラが書き込みを止めてくれます。

```c
const char *b = "hello";
b[0] = 'H';                // コンパイルエラーになる。助かる
```

### 主な文字列関数（`<string.h>`）

```c
size_t strlen(const char *s);                       // 長さ（'\0' は含まない）
char  *strcpy(char *dst, const char *src);          // コピー   ← 危険
char  *strncpy(char *dst, const char *src, size_t n);// 長さ制限つき ← 落とし穴あり
int    strcmp(const char *a, const char *b);        // 比較。等しければ 0
char  *strchr(const char *s, int c);                // 文字を探す
void  *memcpy(void *dst, const void *src, size_t n);// バイト列コピー
void  *memset(void *dst, int c, size_t n);          // バイト列を埋める
```

注意点:

- **`strlen` は O(n)** です。`'\0'` を探して毎回先頭から走ります。
  ループの条件式に `i < strlen(s)` と書くと O(n²) になります。
- **`==` で文字列比較はできません。** ポインタ同士のアドレス比較になってしまいます。
  必ず `strcmp(a, b) == 0` を使ってください。
- **`strcpy` はコピー先の大きさを見ません。** バッファオーバーフローの温床です。

### 文字列を組み立てるなら `snprintf`

```c
char buf[32];
snprintf(buf, sizeof(buf), "SCORE: %d", score);   // 必ずこちら
sprintf(buf, "SCORE: %d", score);                 // サイズを見ない。使わない
```

`snprintf` は書き込むバイト数を `sizeof(buf)` 以下に制限し、必ず `'\0'` で終端してくれます。
**文字列を組むときは常に `snprintf`** と覚えておけば大丈夫です。

---

## 5.6 `printf` の書式

DS でデバッグ表示するときに毎日使うので、主なものを覚えてください。

| 書式 | 対象 | 例 |
| --- | --- | --- |
| `%d` | `int`（符号あり 10 進） | `printf("%d", -5)` → `-5` |
| `%u` | `unsigned`（10 進） | `printf("%u", 5u)` → `5` |
| `%x` / `%X` | 16 進 | `printf("%04X", 255)` → `00FF` |
| `%s` | 文字列（`char *`） | `printf("%s", name)` |
| `%c` | 1 文字 | `printf("%c", 'A')` → `A` |
| `%p` | ポインタ | `printf("%p", ptr)` |
| `%%` | `%` そのもの | |
| `%zu` | `size_t` | `printf("%zu", sizeof(int))` |
| `%f` | `double`（DS では避けたい） | |

幅とゼロ埋めが便利です。

```c
printf("HP:%3d\n", hp);      // "HP:  7"   右詰め 3 桁
printf("%02d:%02d\n", m, s); // "03:07"    ゼロ埋め
printf("addr=%08X\n", addr); // "addr=02000010"
```

**書式と引数の型が食い違うと未定義動作です。**

```c
printf("%d\n", someLongLong);   // 危険
printf("%s\n", 42);             // 42 をアドレスとして読みに行く。クラッシュ
```

`-Wall` は書式文字列をチェックしてくれるので、警告を無視しないでください。

### DS での注意

DS で `printf` を使うには、先にコンソールを初期化する必要があります。

```c
#include <nds.h>
#include <stdio.h>

int main(void)
{
    consoleDemoInit();          // 下画面をテキストコンソールにする
    printf("hello\n");

    while (1) swiWaitForVBlank();
}
```

古い devkitARM 時代の資料には「整数専用の `iprintf` を使え」と書かれていることがあります。
BlocksDS（picolibc）でも `iprintf` は使えますが、通常は `printf` で問題ありません。
ただし `%f` を使うと浮動小数点フォーマットのコードがリンクされ、ROM が大きくなるので、
**デバッグ表示は整数で済ませる**のが習慣です。

---

## 確認問題

**Q1.** 次のコードは何を出力しますか。なぜですか。

```c
void f(int a[10])
{
    printf("%zu\n", sizeof(a));
}

int main(void)
{
    int arr[10];
    printf("%zu\n", sizeof(arr));
    f(arr);
}
```

<details><summary>解答</summary>

DS（32bit）では **`40` と `4`**、64bit PC では **`40` と `8`**。

`main` の中の `arr` は本物の配列なので `sizeof` は 40（= 4 バイト × 10）。
一方、関数の引数の `int a[10]` は**コンパイラによって `int *a` に読み替えられます**。
`sizeof(a)` はポインタのサイズになります。

これが「配列を関数に渡すと長さが失われる」の実演です。
長さは別の引数で渡してください。
</details>

**Q2.** 文字列 `a` と `b` が同じ内容かを調べたい。次のコードは何が間違っていますか。

```c
if (a == b) { ... }
```

<details><summary>解答</summary>

`a` と `b` は `char *`（アドレス）なので、これは
**「2 つのポインタが同じアドレスを指しているか」**を比べています。
内容が同じでも、別々の配列なら偽になります。

```c
if (strcmp(a, b) == 0) { ... }    // 正しい
```

`strcmp` は等しいとき **0** を返す点に注意（真ではなく 0）。
`if (strcmp(a, b))` と書くと「異なるとき」の判定になります。
</details>

**Q3.** 32×24 のタイルマップを 1 次元配列で持ちます。
`(x, y)` のタイルを安全に読む関数を書いてください。画面外は壁（`TILE_WALL`）扱いとします。

<details><summary>解答</summary>

```c
#define MAP_W 32
#define MAP_H 24
#define TILE_WALL 1

static u8 sMap[MAP_W * MAP_H];

static inline u8 mapGet(int x, int y)
{
    if (x < 0 || x >= MAP_W || y < 0 || y >= MAP_H)
        return TILE_WALL;
    return sMap[y * MAP_W + x];
}
```

ポイント:

- **境界チェックを関数の中に閉じ込める**ことで、呼び出し側が毎回書かなくて済む
- `static inline` にすればコストはほぼゼロ
- 添字は `y * MAP_W + x`（行優先）。`x * MAP_H + y` にしないよう注意

</details>

---

[← 前章: 4. ポインタ](04-pointers.md) | [目次](../README.md) | [次章: 6. 構造体と列挙型 →](06-structs.md)
