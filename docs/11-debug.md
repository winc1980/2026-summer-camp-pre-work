# 11. デバッグ

> **この章で分かること**
>
> - 未定義動作のカタログを頭に入れる
> - コンパイラのエラーメッセージを読めるようになる
> - DS でのデバッグ手段を知っておく

**合宿中にこの章を一番読み返すことになります。** ブックマークしておいてください。

---

## 11.1 未定義動作（UB）カタログ

C は、規則違反に対して「何が起きるかは決めない」という立場を取ります。
**エラーにならず、動いているように見えることが多い**のが最悪の性質です。

| # | やってはいけないこと | 症状 |
| --- | --- | --- |
| 1 | 配列の範囲外アクセス | 隣の変数が壊れる。無関係な場所で不具合 |
| 2 | 未初期化変数の読み取り | 実行のたびに結果が変わる |
| 3 | NULL / 無効ポインタの間接参照 | クラッシュ、または謎の値 |
| 4 | 解放後のメモリ使用（use-after-free） | 一見動くが、後で壊れる |
| 5 | 二重 `free` | ヒープの管理情報が壊れる |
| 6 | ローカル変数のアドレスを返す | 呼び出し直後は動く。関数を挟むと壊れる |
| 7 | 符号付き整数のオーバーフロー | 最適化レベルで挙動が変わる |
| 8 | `'\0'` が無い文字列を `strlen` に渡す | メモリの果てまで読む |
| 9 | スタックオーバーフロー | 検出されず、隣のメモリを破壊 |
| 10 | `printf` の書式と引数の型の不一致 | クラッシュ、または文字化け |

### UB が厄介な理由

```c
int arr[5];
arr[7] = 42;        // UB。だが多くの場合、その場では何も起きない
```

`arr` の隣に別の変数があれば、その変数が 42 になります。
**症状が出るのは、その変数が使われるとき**です。
原因のコードと症状の場所が離れているので、デバッガで追うのが極めて難しくなります。

「昨日まで動いていたのに、今日 1 行足したら別の場所が壊れた」というときは、
**足した 1 行ではなく、前から潜んでいた UB が顔を出した**可能性を疑ってください。

---

## 11.2 コンパイラ警告を全部読む

**C において、警告は「将来のバグの予告」です。** 無視しないでください。

BlocksDS のテンプレートには `-Wall` が入っていますが、`-Wextra` を足すことを勧めます。

```makefile
# Makefile
WARNFLAGS := -Wall -Wextra
```

| フラグ | 検出するもの |
| --- | --- |
| `-Wall` | 基本的な間違い（未使用変数、書式の不一致、`if (a = b)` など） |
| `-Wextra` | さらに細かいもの（符号の比較、未使用の引数） |

### 特に危険な警告 3 つ

```
warning: implicit declaration of function 'foo'
```

→ ヘッダの `#include` 漏れ、または関数名のタイプミス。**必ず直す**（3 章）

```
warning: function returns address of local variable
```

→ ダングリングポインタ。**必ず直す**（4 章）

```
warning: comparison of integer expressions of different signedness
```

→ 符号あり／なしの混在。`-1 < 1u` が偽になるやつ（2 章）

---

## 11.3 エラーメッセージの読み方

### 大原則: 一番上のエラーだけを見る

```
main.c:12:5: error: expected ';' before 'return'
main.c:15:1: error: expected declaration specifiers before '}'
main.c:15:1: error: expected '}' at end of input
... （50 行続く）
```

**セミコロンを 1 個忘れただけで、この量が出ます。**
2 個目以降は 1 個目の巻き添えです。**上から 1 個直して、また `make`。** これを繰り返してください。

### よく見るエラーと対処

| メッセージ | 意味 | 対処 |
| --- | --- | --- |
| `expected ';' before ...` | セミコロン忘れ | **エラー行の 1 つ上の行**を見る |
| `'foo' undeclared (first use in this function)` | 未宣言の変数 | 綴り、宣言、`#include` を確認 |
| `implicit declaration of function 'foo'` | 未宣言の関数 | ヘッダを include する |
| `undefined reference to 'foo'` | **リンク**エラー | 実体が無い。ビルド対象・`static`・`LIBS` を確認 |
| `multiple definition of 'foo'` | 定義が複数 | ヘッダに定義を書いていないか（`extern` にする） |
| `dereferencing pointer to incomplete type` | 型の中身が見えていない | その型を定義するヘッダを include |
| `assignment discards 'const' qualifier` | `const` を外そうとしている | 設計を見直す。キャストで潰さない |
| `passing argument 1 of 'foo' from incompatible pointer type` | ポインタの型違い | `&` の付け忘れ・付けすぎが多い |

### `expected ';'` は 1 行上を見る

```c
int x = 5          // ← ここでセミコロン忘れ
return x;          // ← エラーはこの行に出る
```

コンパイラは「`;` が来るはず」の場所まで読み進んでからエラーを出すので、
**報告される行の 1 つ上**が本当の原因です。

---

## 11.4 DS でのデバッグ手段

### (a) `printf` — 一番使う

```c
#include <nds.h>
#include <stdio.h>

int main(void)
{
    consoleDemoInit();       // 下画面をテキストコンソールにする

    while (1) {
        scanKeys();
        swiWaitForVBlank();

        consoleClear();
        printf("x=%3d y=%3d\n", player.x, player.y);
        printf("bullets=%d\n", countActiveBullets());
    }
}
```

コツ:

- **毎フレーム `consoleClear()` してから出す**と、値の変化が見やすい
- 桁を揃える（`%3d`）と、値が動いても行がガタつかない
- `%f` は使わない（ROM が膨らむ、遅い）。固定小数点は `>> 12` して整数で出す

### (b) no$gba スタイルのデバッグ出力

画面を占領せずにログを出せます。melonDS でも動きます。

```c
consoleDebugInit(DebugDevice_NOCASH);
fprintf(stderr, "loaded stage %d\n", stageId);
```

ゲーム画面を見ながらログも取りたいときに便利です。

### (c) アサーション

「ここでは必ずこうなっているはず」を書いておくと、崩れた瞬間に止まってくれます。

```c
#include <assert.h>

void enemyDamage(int index, int dmg)
{
    assert(index >= 0 && index < MAX_ENEMIES);
    ...
}
```

libnds には `sassert()` もあり、こちらはメッセージ付きで画面に表示されます。

**アサーションを有効にするには、デバッグ版のライブラリをリンクします。**
Makefile を編集してください。

```makefile
# デバッグビルド
LIBS := -lmm9 -lnds9d      # nds9 → nds9d（デバッグ版）
```

そして `-DNDEBUG` が付いていないことを確認します（テンプレートでは付いていません）。

### (d) 例外ハンドラ（Guru Meditation）

クラッシュしたときに、レジスタの内容とアドレスを画面に出してくれます。
**入れておかないと、落ちたときに画面が真っ白になるだけで何も分かりません。**

```c
#include <nds.h>

int main(void)
{
    defaultExceptionHandler();     // ← 最初に呼んでおく
    consoleDemoInit();
    ...
}
```

クラッシュ時に表示されるアドレスから、ソースの行を特定できます。

```console
$ arm-none-eabi-addr2line -e build/mygame.elf 0x020045A8
/home/you/mygame/source/enemy.c:87
```

**合宿では最初にこれを入れておいてください。** デバッグ時間が段違いに変わります。

### (e) もっと踏み込む（発展）

| やりたいこと | 方法 |
| --- | --- |
| スタック破壊を検出 | `CFLAGS += -fstack-protector-strong`（デバッグ版ライブラリと併用） |
| 各関数のスタック使用量を見る | `CFLAGS += -fstack-usage` → `build/**/*.su` に出力 |
| ステップ実行する | melonDS の **Config → Emu settings → Devtools** で GDB スタブを有効にし、`gdb-multiarch build/mygame.elf` から `target remote localhost:3333` |

合宿の時間内では `printf` + 例外ハンドラで足りることが多いです。

---

## 11.5 症状別チェックリスト

合宿中に詰まったら、まずここを見てください。

### 画面が真っ白／真っ黒のまま

- [ ] `videoSetMode()` を呼んでいるか
- [ ] `vramSetBankA()` などで VRAM を割り当てているか
- [ ] パレットに色を設定しているか（全部 0 = 黒）
- [ ] `while (1) swiWaitForVBlank();` でループしているか（`main` を抜けていないか）
- [ ] `defaultExceptionHandler()` を入れて、実は落ちていないか確認

### スプライトが表示されない

- [ ] `oamInit(&oamMain, ...)` を呼んだか
- [ ] `oamUpdate(&oamMain)` を毎フレーム呼んでいるか（**忘れがち**）
- [ ] `oamAllocateGfx` の戻り値にグラフィックデータを転送したか
- [ ] スプライトパレットに色が入っているか
- [ ] `oamSet` の `id` が他と重複していないか
- [ ] 座標が画面外になっていないか（固定小数点を `>> 12` し忘れ）

### 動きがカクつく／遅い

- [ ] `float` を毎フレーム大量に使っていないか（9 章）
- [ ] `/` や `%` をループの中で使っていないか
- [ ] `strlen` をループ条件に書いていないか
- [ ] 二重ループの回数が多すぎないか
- [ ] `swiWaitForVBlank()` を 1 フレームに 2 回呼んでいないか（30fps になる）

### 変な値になる／たまに落ちる

- [ ] 配列の添字が範囲内か（`<=` と `<` の間違い）
- [ ] ローカル変数を初期化したか
- [ ] ローカル配列が大きすぎないか（7 章）
- [ ] `u8` / `u16` に大きな値を入れて切り詰められていないか（2 章）
- [ ] 符号なしの引き算で負になっていないか（2 章）
- [ ] 固定小数点の乗算で `>> 12` を忘れていないか（9 章）
- [ ] `printf` の書式と引数の型が合っているか

### ビルドが通らない

- [ ] エラーメッセージの**一番上**を読んだか
- [ ] `expected ';'` なら、その 1 行上を見たか
- [ ] `undefined reference` なら、そのファイルが `source/` にあるか
- [ ] ヘッダに変数の定義を書いていないか
- [ ] `make clean` してから `make` してみたか

### melonDS で動かない

- [ ] BIOS / ファームウェアの設定は済んでいるか（無くても大抵動く）
- [ ] `.nds` ファイルが実際に生成されているか（`ls -la *.nds`）
- [ ] ファイルシステム（NitroFS）を使うなら、ROM に組み込まれているか

---

## 11.6 合宿前に仕込んでおくと得するもの

`main.c` の冒頭に、これを入れておいてください。

```c
#include <nds.h>
#include <stdio.h>

int main(void)
{
    defaultExceptionHandler();       // クラッシュ時に情報を出す
    consoleDemoInit();               // printf デバッグを可能にする

    // ... ゲーム本体 ...
}
```

Makefile はデバッグ寄りに倒しておきます（速度が足りなくなったら戻す）。

```makefile
WARNFLAGS := -Wall -Wextra
LIBS      := -lmm9 -lnds9d          # デバッグ版 libnds（assert が効く）
```

そして `util.h` にこれを入れておくと便利です。

```c
#ifdef DEBUG
#  define DBG(...) printf(__VA_ARGS__)
#else
#  define DBG(...) ((void)0)
#endif
```

`DBG("hp=%d\n", hp);` と書いておけば、リリース時に `DEBUG` を外すだけで
全部のデバッグ出力が消えます。

---

## 確認問題

**Q1.** 次のエラーが出ました。何行目に問題がある可能性が高いですか。

```
main.c:20:5: error: expected ';' before 'return'
```

<details><summary>解答</summary>

**19 行目**（またはそれより上）の可能性が高いです。

コンパイラは「セミコロンが来るはずの場所」を読み飛ばして、
次のトークン（`return`）に到達したところでエラーを報告します。
つまり、セミコロンを付け忘れたのは**報告された行の 1 つ前の文**です。
</details>

**Q2.** 「昨日まで動いていたコードに 1 行 `printf` を足したら、まったく別の場所で落ちるようになった」。
何が起きている可能性がありますか。

<details><summary>解答</summary>

**元から潜んでいた未定義動作が顔を出した**可能性が高いです。

典型的なのは配列の範囲外書き込みで、
「隣にたまたま無害な変数があったので何も起きていなかった」状態から、
1 行足したことで変数の配置が変わり、重要なデータを壊すようになった、というパターンです。

`printf` の追加自体はほぼ無害なので、それを疑うより:

- 配列の添字（`<=` と `<`）
- ローカル配列のサイズ（スタックオーバーフロー）
- 未初期化変数

を先に洗ってください。`-fstack-protector-strong` と
デバッグ版 libnds（`-lnds9d`）でのビルドも試す価値があります。
</details>

**Q3.** DS でスプライトが表示されないとき、確認すべき項目を 4 つ挙げてください。

<details><summary>解答</summary>

1. `oamUpdate(&oamMain)` を毎フレーム呼んでいるか（`oamSet` だけでは反映されない）
2. `oamInit()` と `vramSetBankA(VRAM_A_MAIN_SPRITE)` などの VRAM 割り当てをしたか
3. スプライトパレットに色が入っているか（全部 0 だと黒＝背景に紛れる）
4. 座標が画面内か（固定小数点のまま `oamSet` に渡していないか。`>> 12` の忘れ）

加えて、`oamSet` の `id` の重複、`oamAllocateGfx` の戻り値へのデータ転送忘れもよくあります。
</details>

**Q4.** `defaultExceptionHandler()` を呼んでおくべき理由を説明してください。

<details><summary>解答</summary>

呼んでいないと、プログラムがクラッシュしたときに**画面が固まるか真っ白になるだけ**で、
どこで何が起きたか一切分かりません。

`defaultExceptionHandler()` を呼んでおくと、クラッシュ時に
「Guru Meditation Error」画面が出て、CPU レジスタの内容と
**クラッシュしたアドレス**が表示されます。そのアドレスから

```console
arm-none-eabi-addr2line -e build/mygame.elf 0x020045A8
```

でソースファイルと行番号を特定できます。
短期決戦の合宿では、この差が数時間の違いになります。
</details>

---

[← 前章: 10. ゲームコードの型](10-game-patterns.md) | [目次](../README.md) | [次章: 12. 開発環境 →](12-devcontainer.md)
