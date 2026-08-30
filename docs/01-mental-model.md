# 1. C の世界観とビルドの仕組み

> **この章で分かること**
>
> - C が他の言語とどう違うか
> - `#include` が何をしているか
> - エラーメッセージの 2 大分類

---

## 1.1 C は安全装置がない言語

Python や JavaScript は、言語処理系が面倒を見ながらコードを実行します。
配列の範囲外を触れば例外が飛び、要らないメモリは GC が回収してくれます。

**C にはそれがありません。** 書いたコードはほぼそのまま機械語になり、CPU が実行します。

DS の CPU は 67MHz、メモリは 4MB です。スマホの数十分の一の速度で、
メモリは 1000 分の 1 以下。この上で 60fps を出すために C を使います。

### 他の言語との違い

| 他の言語 | C |
| --- | --- |
| GC がメモリを回収 | 自分で管理する（そもそも動的確保をあまり使わない） |
| 配列外アクセスで例外 | **何も起きない。隣の変数を壊して先に進む** |
| `try` / `catch` | 例外が無い。エラーは戻り値で返す |
| 文字列は組み込みの型 | ただの `char` の配列 |
| `import` / モジュール | ヘッダファイルをテキストとして貼り付ける |
| リストや辞書が使える | 配列しかない |

**2 行目が一番大事です。**

```c
int arr[5];
arr[10] = 99;      // エラーにならない。隣のメモリが書き換わるだけ
```

C にはこういう「やってはいけないが、止めてもくれない」操作がたくさんあります。
これを **未定義動作（UB）** と呼びます。
UB を踏んだプログラムは、クラッシュするかもしれないし、
**何事もなく動いて 3 時間後に別の場所で壊れる**かもしれません。

---

## 1.2 ソースコードが .nds になるまで

```
main.c ──[1]プリプロセス──▶ [2]コンパイル ──▶ main.o ─┐
player.c ─────────────同じ────────────▶ player.o ─┤
                                                    ├─[3]リンク─▶ .elf ─▶ .nds
                                        libnds.a ───┘
```

### [1] プリプロセス — ただのテキスト処理

`#` で始まる行を処理します。**C の文法はまだ関係ありません。**

`#include <nds.h>` は「**この位置に `nds.h` の中身をまるごと貼り付けろ**」という指示です。
それだけです。この一点さえ押さえておけば、ヘッダまわりの話は全部つながります。

### [2] コンパイル — .c ファイル 1 個ずつ、独立に

コンパイラは **`.c` を 1 個ずつ、他のファイルを知らないまま**機械語にします。

つまり `main.c` をコンパイルしている最中、コンパイラは `player.c` の中身を知りません。
だから `main.c` から `playerUpdate()` を呼びたければ、
**「そういう名前の関数がどこかにある」と先に教えておく**必要があります。

それが **プロトタイプ宣言** で、それを書いておく場所がヘッダファイル（`.h`）です。

### [3] リンク — バラバラの .o をつなぐ

全部の `.o` を集めて、「関数を呼んでいる場所」と「関数の実体」をつなぎます。
実体が見つからないとこうなります。

```
undefined reference to `playerUpdate'
```

### エラーの 2 大分類

| 種類 | いつ出るか | 代表例 |
| --- | --- | --- |
| **コンパイルエラー** | 文法がおかしい | `expected ';' before ...` |
| **リンクエラー** | 文法は OK だが実体が無い | `undefined reference to ...` |

`undefined reference` が出たら、疑うのはこの 3 つです。

- そのファイルを `source/` に置き忘れている
- 関数名のタイプミス
- Makefile の `LIBS` にライブラリの指定が足りない

---

## 1.3 ヘッダファイルの書き方

`.h` には **宣言だけ**、`.c` に **実体** を書きます。

```c
/* player.h --------------------------------- */
#ifndef PLAYER_H          // ← インクルードガード（後述）
#define PLAYER_H

#include <nds.h>

typedef struct {
    int x, y;
    int hp;
} Player;

void playerInit(Player *p);
void playerUpdate(Player *p, u32 keys);

#endif
```

```c
/* player.c --------------------------------- */
#include "player.h"

void playerInit(Player *p)
{
    p->x  = 128;
    p->y  = 96;
    p->hp = 3;
}

void playerUpdate(Player *p, u32 keys)
{
    if (keys & KEY_LEFT)  p->x -= 1;
    if (keys & KEY_RIGHT) p->x += 1;
}
```

### インクルードガードが必要な理由

`#include` はテキストの貼り付けでした。
`main.c` が `player.h` と `enemy.h` を include していて、
`enemy.h` の中でも `player.h` を include していたら、
**`player.h` の中身が 2 回貼り付けられます。** 型の定義が 2 回出てきてエラーです。

`#ifndef PLAYER_H` は「`PLAYER_H` がまだ定義されていなければ以下を有効にする」なので、
2 回目はまるごとスキップされます。**おまじないとして毎回書いてください。**

`#pragma once` という 1 行で済む書き方もあり、GCC で使えます。どちらでも構いません。

### `<...>` と `"..."`

| 書き方 | 用途 |
| --- | --- |
| `#include <nds.h>` | ライブラリ・システムのヘッダ |
| `#include "player.h"` | 自分で書いたヘッダ |

---

## 1.4 マクロ

### 定数

```c
#define SCREEN_WIDTH  256
#define MAX_ENEMIES   32
```

単なるテキスト置換です。`SCREEN_WIDTH` と書いた場所が `256` に置き換わります。

### 関数っぽいマクロは括弧を忘れない

```c
#define SQUARE(x) x * x         // ❌ 悪い例

int a = SQUARE(2 + 3);          // 2 + 3 * 2 + 3 → 11 になってしまう
```

```c
#define SQUARE(x) ((x) * (x))   // ✅ 引数と全体を括弧で囲む
```

**そもそも、関数で書けるものは関数で書いてください。** 速度も変わりません。

```c
static inline int square(int x) { return x * x; }
```

---

## 1.5 最小のプログラム

```c
#include <stdio.h>

int main(void)
{
    printf("hello, world\n");
    return 0;
}
```

手元で試すなら（DS 用ではない普通の `gcc` で動きます）:

```bash
gcc -Wall -Wextra -o hello hello.c
./hello
```

**`-Wall -Wextra` は常に付けてください。**
C の警告は「動くけど間違っている」を教えてくれる、数少ない安全装置です。

---

## 1.6 BlocksDS ではこうなっている

Makefile が大体こういうことをしています。

```makefile
SOURCEDIRS := source                     # このフォルダの .c を全部拾う
ARCH       := -mthumb -mcpu=arm946e-s+nofp
CFLAGS     += -Wall ... $(ARCH) -O2
LIBS       := -lmm9 -lnds9               # maxmod と libnds をリンク
```

- **`source/` に `.c` を置けば自動でビルドされます。** Makefile を編集する必要はありません
- `-mcpu=arm946e-s+nofp` の **`nofp` は「FPU なし」**。9 章で効いてきます
- 最後に `ndstool` が `.nds` ROM に固めます

---

## 確認問題

**Q1.** `#include "player.h"` は具体的に何をしますか。

<details><summary>解答</summary>

プリプロセッサが、その行を `player.h` の中身のテキストで置き換えます。
C の文法チェックはその後なので、`#include` 自体は純粋なテキスト操作です。
</details>

**Q2.** `undefined reference to 'enemyUpdate'` が出ました。何を疑いますか。

<details><summary>解答</summary>

これは**リンクエラー**です。宣言（ヘッダ）は見つかったので文法チェックは通ったが、
`enemyUpdate` の**実体**がどこにも無かった、という意味です。

- `enemy.c` を作ったが `source/` に置いていない
- 関数名や引数がヘッダの宣言と食い違っている
- ライブラリ関数なら Makefile の `LIBS` の指定が足りない

</details>

---

[目次](../README.md) | [次章: 2. 型と数値 →](02-types.md)
