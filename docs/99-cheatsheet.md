# C / BlocksDS チートシート

合宿中はこのページを開いておいてください。詳細は各章へのリンクから。

---

## 型（DS = ARM 32bit）

| 型 | サイズ | libnds 別名 |
| --- | --- | --- |
| `char` | 1 | — （**ARM では unsigned**） |
| `int8_t` / `uint8_t` | 1 | `s8` / `u8` |
| `int16_t` / `uint16_t` | 2 | `s16` / `u16` |
| `int32_t` / `uint32_t` | 4 | `s32` / `u32` |
| `int` / `long` | 4 | — |
| `long long` | 8 | `s64` / `u64` |
| ポインタ | 4 | — |
| `float` / `double` | 4 / 8 | **FPU 無し。避ける** |

`volatile` 版は `vu8` / `vu16` / `vu32`（ハードウェアレジスタ用）。

→ [2 章](02-types.md)

---

## ポインタ

```c
int  x = 42;
int *p = &x;      // & = アドレスを取る
*p = 100;         // * = 指す先にアクセス
x;                // 100 になっている

p->member         // (*p).member の省略形
```

| 書き方 | 意味 |
| --- | --- |
| `const T *p` | **指す先**が const（読み取り専用引数。これが 99%） |
| `T *const p` | **ポインタ自身**が const |

**out 引数のイディオム**

```c
void f(int *out) { *out = 42; }
int v; f(&v);
```

→ [4 章](04-pointers.md)

---

## 配列

```c
int a[5] = {0};                       // 全部 0
size_t n = sizeof(a) / sizeof(a[0]);  // 要素数（本物の配列でのみ有効）

a[i] == *(a + i)                      // 同じ意味
```

- **関数に渡すと長さが失われる** → 長さも引数で渡す
- **境界チェックは無い** → 範囲外は隣のメモリを壊す
- 多次元配列は行優先。`m[y][x]` ≡ `m[y * W + x]`

→ [5 章](05-arrays-strings.md)

---

## 文字列

```c
char       a[] = "hi";     // 書き換え可（3 バイト: 'h','i','\0'）
const char *b  = "hi";     // リテラル。書き換え不可

strlen(s)                  // 長さ（O(n)。ループ条件に書かない）
strcmp(a, b) == 0          // 内容が等しい（== では比較できない）
snprintf(buf, sizeof(buf), "%d", n);   // 文字列組み立ては常にこれ
```

**書式**: `%d` int / `%u` unsigned / `%s` 文字列 / `%c` 文字 / `%x` 16進 /
`%p` ポインタ / `%zu` size_t / `%3d` 幅 3 / `%02d` ゼロ埋め

→ [5 章](05-arrays-strings.md)

---

## 構造体

```c
typedef struct {
    int x, y;
} Point;

Point p = { .x = 1, .y = 2 };      // 指定初期化子（推奨）
p = (Point){ .x = 3 };             // 複合リテラル。書かないメンバは 0

void f(const Point *p);            // 読むだけ
void g(Point *p);                  // 書き換える
```

- 構造体は**値でコピーされる**（配列と違う）
- **メンバは大きい順に並べる**（パディング削減）
- ARM では非アライメントアクセスが壊れる → `memcpy` を使う

→ [6 章](06-structs.md)

---

## メモリ

| 種類 | 置き場所 | 寿命 | 初期化 |
| --- | --- | --- | --- |
| グローバル / `static` | `.data` / `.bss` | プログラム全体 | **0 で初期化される** |
| ローカル | スタック（DTCM 16KB） | ブロックを抜けるまで | **されない（ゴミ）** |
| `malloc` | ヒープ | `free` まで | されない（`calloc` は 0） |

**DS のメモリ**: メイン RAM 4MB / VRAM 656KB / **スタックは 16KB 程度**

```c
void f(void) {
    u8 buf[64 * 1024];        // ❌ スタックが溢れる
    static u8 buf[64 * 1024]; // ✅ .bss に置く
}
```

**ゲームループ中は `malloc` しない。** 固定長配列 + `active` フラグで管理。

→ [7 章](07-memory.md)

---

## ビット演算

```c
flags |=  BIT(n);        // 立てる
flags &= ~BIT(n);        // 下ろす（~ を忘れない）
flags ^=  BIT(n);        // 反転
if (flags & BIT(n))      // 判定

(v >> 6) & 0x03          // ビット 6-7 を取り出す
x << 3                   // x * 8
x >> 3                   // x / 8
```

**優先順位に注意**: `flags & BIT(3) == 0` は誤り → `(flags & BIT(3)) == 0`

**`volatile`**: ハードウェアレジスタ／割り込みと共有する変数に必須

→ [8 章](08-bitops.md)

---

## 固定小数点（12bit）

```c
#define FP 12
#define TO_FP(n)  ((n) << FP)      // 1.0 → 4096
#define TO_INT(n) ((n) >> FP)

a + b                              // 加減算はそのまま
(a * b) >> FP                      // 乗算は掛けてからシフト
((s64)a << FP) / b                 // 除算はシフトしてから割る
```

**libnds**（`nds/arm9/math.h`）

```c
inttof32(n)  f32toint(n)  floattof32(n)  f32tofloat(n)
mulf32(a,b)  divf32(a,b)  sqrtf32(a)     div32(a,b)
```

**三角関数**（`nds/arm9/trig_lut.h`）— 一周 = 32768

```c
s16 s = sinLerp(angle);            // 戻り値は 4096 = 1.0
s16 c = cosLerp(angle);
angle = degreesToAngle(45);
angle = (angle + 200) & (DEGREES_IN_CIRCLE - 1);   // ラップ
```

→ [9 章](09-fixed-point.md)

---

## ゲームループ

```c
int main(void)
{
    defaultExceptionHandler();
    videoSetMode(MODE_0_2D);
    consoleDemoInit();
    gameInit();

    while (1) {
        scanKeys();
        u32 held = keysHeld();
        u32 down = keysDown();

        gameUpdate(held, down);     // 状態を更新（VRAM は触らない）

        swiWaitForVBlank();         // 60fps に同期

        gameDraw();                 // VBlank 中に転送
        oamUpdate(&oamMain);
    }
}
```

→ [10 章](10-game-patterns.md)

---

## 入力（libnds）

```c
scanKeys();                  // 毎フレーム 1 回だけ
u32 h = keysHeld();          // 押されている（移動用）
u32 d = keysDown();          // 押した瞬間（ジャンプ・決定用）
u32 u = keysUp();            // 離した瞬間

KEY_A KEY_B KEY_X KEY_Y KEY_L KEY_R
KEY_UP KEY_DOWN KEY_LEFT KEY_RIGHT
KEY_START KEY_SELECT KEY_TOUCH KEY_LID

touchPosition t;
touchRead(&t);
// t.px, t.py が画面座標
```

---

## 色（BGR555）

```c
RGB15(r, g, b)          // 各 0〜31
ARGB16(a, r, g, b)      // a は 0/1
RGB8(r, g, b)           // 各 0〜255 から変換

BG_PALETTE[1]     = RGB15(31, 0, 0);
SPRITE_PALETTE[1] = RGB15(0, 31, 0);
```

---

## スプライト（libnds OAM）

```c
oamInit(&oamMain, SpriteMapping_1D_32, false);
u16 *gfx = oamAllocateGfx(&oamMain, SpriteSize_16x16,
                          SpriteColorFormat_256Color);
// gfx にグラフィックデータを転送

oamSet(&oamMain, id, x, y,
       priority, palette,
       SpriteSize_16x16, SpriteColorFormat_256Color,
       gfx, -1, false, false, false, false, false);

oamSetXY(&oamMain, id, x, y);
oamSetHidden(&oamMain, id, true);

oamUpdate(&oamMain);         // ← 毎フレーム。忘れると何も出ない
```

---

## Dev Container

```bash
# VS Code:  F1 → Dev Containers: Reopen in Container
#           F1 → Dev Containers: Rebuild Container   （Dockerfile を変えたら）

make                  # ビルド
make clean            # 掃除
make VERBOSE=1        # 実行コマンドを表示

echo $BLOCKSDS        # /opt/wonderful/thirdparty/blocksds/core
ls $BLOCKSDS/examples/                        # サンプル一覧
ls $BLOCKSDS/templates/                       # テンプレート
grep -rn "oamSet" $BLOCKSDS/libs/libnds/include/nds/   # libnds の使い方を調べる
```

`.nds` はホスト側のフォルダにも出るので、**melonDS はホストで起動**します。

**Makefile で触る場所**（上の User config だけ）

```makefile
NAME       := mygame
GAME_TITLE := My Game
SOURCEDIRS := source          # ここ以下の .c は自動でビルドされる
COMPDB     := 1               # compile_commands.json を出す（補完用）
LIBS       := -lmm9 -lnds9    # デバッグ時は -lnds9d
```

→ [12 章](12-devcontainer.md)

---

## デバッグ

```c
defaultExceptionHandler();          // クラッシュ時に情報表示（最初に呼ぶ）
consoleDemoInit();                  // printf を使えるようにする
consoleClear();                     // 毎フレーム消してから出すと見やすい
consoleSetCursor(NULL, x, y);

consoleDebugInit(DebugDevice_NOCASH);
fprintf(stderr, "log\n");           // 画面を占領せずにログ

assert(cond);                       // -lnds9d でリンクすると有効
```

```bash
# コンパイラは PATH に無いので足す
export PATH=$WONDERFUL_TOOLCHAIN/toolchain/gcc-arm-none-eabi/bin:$PATH

arm-none-eabi-addr2line -e build/mygame.elf 0x020045A8   # アドレス → 行番号
arm-none-eabi-size build/mygame.elf                      # text/data/bss のサイズ
```

**Makefile**

```makefile
WARNFLAGS := -Wall -Wextra
CFLAGS    += -fstack-protector-strong -fstack-usage
LIBS      := -lmm9 -lnds9d
```

→ [11 章](11-debug.md)

---

## エラーメッセージ早見

| メッセージ | 意味 |
| --- | --- |
| `expected ';' before ...` | **1 行上**にセミコロン忘れ |
| `implicit declaration of function` | `#include` 漏れ or 綴り違い |
| `undefined reference to` | **リンク**エラー。実体が無い |
| `multiple definition of` | ヘッダに定義を書いている |
| `dereferencing pointer to incomplete type` | 型を定義するヘッダを include |
| `missing separator` (Makefile) | インデントがスペース。**タブ**にする |

**エラーは一番上から 1 個ずつ直す。** 2 個目以降は巻き添え。

---

## よく踏む地雷

- [ ] `<=` と `<` の取り違え（配列の範囲外）
- [ ] ローカル変数を初期化していない
- [ ] ローカルに大きな配列（スタックオーバーフロー）
- [ ] `u8`/`u16` に大きな値を入れて切り詰め
- [ ] 符号なしの引き算が負になってラップアラウンド
- [ ] 固定小数点の乗算で `>> 12` を忘れる
- [ ] `oamUpdate()` を呼び忘れてスプライトが出ない
- [ ] `swiWaitForVBlank()` を忘れて CPU 全力回転
- [ ] `scanKeys()` を呼び忘れて入力が効かない
- [ ] `flags & BIT(n) == 0` の括弧忘れ
- [ ] ヘッダに変数の定義を書いてリンクエラー
- [ ] `strcpy` でバッファオーバーフロー（`snprintf` を使う）

---

[← 目次に戻る](../README.md)
