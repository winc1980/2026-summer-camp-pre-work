# DS 合宿 事前資料 — C言語 速習テキスト

夏合宿で **BlocksDS** を使って Nintendo DS のゲームを作ります。
合宿の数日間を「DS とゲームの中身」に使えるように、
**C でつまずく部分を先に潰しておく**ための教科書です。

---

## 対象読者

何かの言語（Python / JavaScript / Java など）で、変数・条件分岐・ループ・関数・配列を
書いたことがある人。C は初めてでかまいません。

`if` や `for` の説明はしません。**他の言語に無くて C にだけある話**に絞ってあります。

## 開発環境

**Docker + VS Code Dev Containers** を使います。
C のコンパイラや BlocksDS SDK はコンテナの中に入っているので、ローカルには入れません。

ただし、**その土台になるものはローカルに入れる必要があります。**

- Docker Desktop
- VS Code ＋ Dev Containers 拡張
- melonDS（DS エミュレータ）
- **Windows の人は WSL2**

→ 手順は [12 章](docs/12-devcontainer.md) にあります。**合宿前にまずこれを終わらせてください。**

---

## 読む順番

### ステップ 1 — 合宿前に必ず（半日）

| | |
|---|---|
| [12 章 開発環境](docs/12-devcontainer.md) | Docker で Hello, DS! を出すところまで |
| [1 章 C の世界観](docs/01-mental-model.md) | C はどういう言語か／`#include` の正体 |

### ステップ 2 — ここが山場（1〜2 日）

| | |
|---|---|
| [4 章 ポインタ](docs/04-pointers.md) | **最重要。** ここだけは飛ばさないでください |
| [5 章 配列と文字列](docs/05-arrays-strings.md) | 配列＝ポインタ／境界チェックが無い世界 |
| [2 章 型と数値](docs/02-types.md) | 型のサイズ／オーバーフロー |

### ステップ 3 — 余裕があれば

| | |
|---|---|
| [3 章 関数まわり](docs/03-functions.md) | 値渡し／`static` |
| [6 章 構造体](docs/06-structs.md) | ゲームのデータの持ち方 |
| [7 章 メモリ](docs/07-memory.md) | スタックとヒープ／DS のメモリ量 |
| [8 章 ビット演算](docs/08-bitops.md) | キー入力と色を扱うのに必要 |

### ステップ 4 — 合宿中に必要になったら

| | |
|---|---|
| [9 章 固定小数点](docs/09-fixed-point.md) | なめらかな動きを作るとき |
| [10 章 ゲームコードの型](docs/10-game-patterns.md) | ゲームループとエンティティ管理 |
| [11 章 デバッグ](docs/11-debug.md) | **詰まったらここ。** 症状別チェックリストつき |
| [チートシート](docs/99-cheatsheet.md) | 合宿中はこれを開いておく |

> **全部読まなくて大丈夫です。**
> ステップ 1 と 2 だけでも、合宿にはだいぶ楽に入れます。
> 各章の中で `（発展）` と付いた節は、**最初は飛ばしてください。**

---

## 手を動かす

[examples/](examples/) に、C の文法を確かめるための実験プログラムがあります。
読むだけだと必ず抜けるので、動かしてみてください。

```bash
cd examples
./build.sh 02      # 02_ で始まるものをビルドして実行
```

**DS 用ではなく普通の `gcc` でビルドします。** Dev Container の中でも動きますし、
ホストに `gcc` があればホストでも動きます。DS の実機やエミュレータは不要です。

---

## 合宿前チェックリスト

- [ ] Dev Container が起動して、`make` で `.nds` が作れる
- [ ] その `.nds` が melonDS で起動して、画面に文字が出る
- [ ] `int *p = &x;` の `*` と `&` が何をしているか説明できる
- [ ] 関数に配列を渡すと長さが分からなくなる理由を説明できる
- [ ] `u8` に 300 を入れるとどうなるか分かる

---

## リンク

- [BlocksDS 公式ドキュメント](https://blocksds.skylyrac.net/) / [チュートリアル](https://blocksds.skylyrac.net/tutorial/)
- [libnds ソース](https://github.com/blocksds/libnds) — 迷ったらヘッダを読むのが一番早い
- [GBATEK](https://problemkaputt.de/gbatek.htm) — DS ハードウェアの仕様書
