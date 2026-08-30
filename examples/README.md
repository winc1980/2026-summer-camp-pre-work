# サンプルコード

各章の内容を**実際に動かして確かめる**ためのプログラムです。
読むだけだと必ず抜けるので、写経して動かしてください。

**DS 用ではなく、普通の `gcc`（x86 用）でビルドします。**
DS の実機やエミュレータは不要です。

## どこで動かすか

| 場所 | 条件 |
|---|---|
| **Dev Container の中** | そのまま動きます（Dockerfile に `gcc libc6-dev` を入れてあります） |
| **ホストの PC** | `gcc` が入っていれば動きます |

ホストに `gcc` が無い場合:

```bash
sudo apt install build-essential     # Ubuntu / WSL
xcode-select --install               # macOS
```

Windows でホストに入れるのは面倒なので、**Dev Container の中で動かすのが楽です。**

## ビルドと実行

```bash
./build.sh          # 全部ビルド（bin/ に出力）
./build.sh 04       # 04_ で始まるものをビルドして実行
```

個別にやるなら:

```bash
gcc -std=c11 -Wall -Wextra -o bin/01_types 01_types.c
./bin/01_types
```

## 一覧

| ファイル | 対応する章 | 内容 |
|---|---|---|
| [01_types.c](01_types.c) | [2 章](../docs/02-types.md) | 型のサイズ／整数昇格／符号なしのラップ／整数除算／`char` の符号 |
| [02_pointers.c](02_pointers.c) | [4 章](../docs/04-pointers.md) | `&` と `*`／out 引数／`swap`／ポインタ演算 |
| [03_arrays_strings.c](03_arrays_strings.c) | [5 章](../docs/05-arrays-strings.md) | 配列の減衰／`a[i]==*(a+i)`／`'\0'` 終端／`strcmp`／`snprintf`／境界チェック |
| [04_structs.c](04_structs.c) | [6 章](../docs/06-structs.md) | パディング／値コピー／`->`／オブジェクトプール／`enum` とビットフラグ |
| [05_bitops.c](05_bitops.c) | [8 章](../docs/08-bitops.md) | セット/クリア/トグル/テスト／マスク／優先順位の罠／BGR555 |
| [06_fixed_point.c](06_fixed_point.c) | [9 章](../docs/09-fixed-point.md) | 12bit 固定小数点の四則／オーバーフロー／弾道／摩擦／当たり判定 |
| [07_game_loop.c](07_game_loop.c) | [10 章](../docs/10-game-patterns.md) | ゲームループ／状態機械／オブジェクトプール（テキストで描画） |

## PC と DS で結果が変わるところ

`01_types.c` を実行すると分かりますが、手元の PC（64bit）と DS（ARM 32bit）では
一部の値が違います。**これ自体が学ぶべきポイント**です。

| | PC (x86-64 Linux) | DS (ARM 32bit) |
|---|---|---|
| `sizeof(long)` | 8 | **4** |
| `sizeof(void *)` | 8 | **4** |
| 素の `char` | signed（-128〜127） | **unsigned（0〜255）** |

`01_types.c` の最後にある `char 127+1` は、PC では `-128`、DS では `128` になります。
「PC で動いたコードが DS で壊れる」の典型例です。

## わざと警告を出しているところ

`-Wall -Wextra` でビルドすると、次の 3 つの警告が出ます。**これは意図的です。**
「C コンパイラはこういう罠をちゃんと教えてくれる」という実演なので、消さないでください。

| ファイル | 警告 | 教えていること |
|---|---|---|
| `01_types.c` | `comparison of integer expressions of different signedness` | 符号あり/なしの比較（[2 章](../docs/02-types.md#25-符号あり符号なしの混在)） |
| `03_arrays_strings.c` | `'sizeof' on array function parameter` | 配列の減衰（[5 章](../docs/05-arrays-strings.md)） |
| `03_arrays_strings.c` | `'%s' directive output truncated` | `snprintf` が安全に打ち切ってくれている |

**自分のコードで出た警告は、全部潰してください。** 警告は将来のバグの予告です。

## DS 版はどこにある？

このディレクトリのサンプルは、DS のライブラリ（libnds）を使わずに
C の文法だけを確かめるためのものです。**Dev Container の外でも動きます。**

実際に DS で動くコードは [12 章](../docs/12-devcontainer.md) の Hello, DS! と、
BlocksDS 付属のサンプル集を見てください（Dev Container の中で）。

```bash
ls $BLOCKSDS/examples/
```
