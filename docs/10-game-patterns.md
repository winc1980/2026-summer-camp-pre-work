# 10. ゲームコードの型（かた）

> **この章で分かること**
>
> - DS のゲームループの骨格を書ける
> - エンティティを構造体配列で管理できる
> - `enum` + `switch` でゲーム状態を切り替えられる
> - 機能ごとにファイルを分割できる

C にはクラスもモジュールもありません。
その代わり、**限られた道具でどう構造を作るか**という定番の型があります。
合宿でチーム開発するなら、ここを共通言語にしておくと衝突が減ります。

---

## 10.1 ゲームループの骨格

DS のゲームループは、**画面の垂直帰線期間（VBlank）に同期させる**のが基本です。
DS の画面は毎秒 60 回書き換わるので、VBlank を待てば自動的に 60fps になります。

```c
#include <nds.h>
#include <stdio.h>

int main(void)
{
    //--- 初期化（1 回だけ） ---
    videoSetMode(MODE_0_2D);
    consoleDemoInit();
    gameInit();

    //--- メインループ ---
    while (1)
    {
        //--- 1. 入力を取る ---
        scanKeys();
        u32 held = keysHeld();
        u32 down = keysDown();

        //--- 2. 状態を更新する（描画はしない） ---
        gameUpdate(held, down);

        //--- 3. 次のフレームの開始を待つ ---
        swiWaitForVBlank();

        //--- 4. VBlank 中に描画データを転送する ---
        gameDraw();
        oamUpdate(&oamMain);
    }

    return 0;
}
```

### なぜ「更新」と「描画」を分けるのか

VRAM や OAM への書き込みは、**画面を描いていない VBlank 期間中**に行わないと
画面がちらつきます（ティアリング）。

そのため、

- **更新フェーズ**: ゲームの状態（座標、HP、スコア）を計算する。VRAM は触らない
- **描画フェーズ**: 計算済みの状態を、VBlank 中に一気に VRAM/OAM へ流し込む

と分けます。この分離は、コードの見通しの点でも有利です。

### `swiWaitForVBlank()` を必ず入れる

これが無いと、CPU が全力でループを回し続けます。
バッテリーを食うだけでなく、描画のタイミングも合いません。**必ず入れてください。**

---

## 10.2 エンティティ管理 — 構造体配列 + active フラグ

6 章でも触れた、C ゲームコードの最重要パターンです。改めてまとめます。

```c
/* bullet.h ---------------------------------------------------- */
#ifndef BULLET_H
#define BULLET_H

#include <nds.h>

void bulletInit(void);
void bulletSpawn(int x, int y, int vx, int vy);
void bulletUpdate(void);
void bulletDraw(void);
int  bulletCheckHit(int x, int y, int r);

#endif
```

```c
/* bullet.c ---------------------------------------------------- */
#include <string.h>
#include "bullet.h"

#define MAX_BULLETS 32
#define FP 12

typedef struct {
    int  x, y;          // 固定小数点
    int  vx, vy;
    bool active;
} Bullet;

// static → このファイルの外からは見えない（C における private）
static Bullet sBullets[MAX_BULLETS];
static u16   *sGfx;

void bulletInit(void)
{
    memset(sBullets, 0, sizeof(sBullets));
    sGfx = oamAllocateGfx(&oamMain, SpriteSize_16x16,
                          SpriteColorFormat_256Color);
    // sGfx にグラフィックデータを転送する処理（省略）
}

void bulletSpawn(int x, int y, int vx, int vy)
{
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (sBullets[i].active) continue;

        sBullets[i] = (Bullet){
            .active = true,
            .x = x, .y = y, .vx = vx, .vy = vy,
        };
        return;             // 空きが見つかったら終わり
    }
    // 空きが無ければ何もしない。弾が出ないだけでクラッシュはしない
}

void bulletUpdate(void)
{
    for (int i = 0; i < MAX_BULLETS; i++) {
        Bullet *b = &sBullets[i];       // ポインタを取ると読みやすい
        if (!b->active) continue;

        b->x += b->vx;
        b->y += b->vy;

        int px = b->x >> FP;
        int py = b->y >> FP;
        if (px < -16 || px > 256 || py < -16 || py > 192)
            b->active = false;
    }
}

void bulletDraw(void)
{
    for (int i = 0; i < MAX_BULLETS; i++) {
        const Bullet *b = &sBullets[i];

        if (!b->active) {
            oamSetHidden(&oamMain, OAM_BULLET_BASE + i, true);
            continue;
        }
        oamSet(&oamMain, OAM_BULLET_BASE + i,
               b->x >> FP, b->y >> FP,
               0,                            // priority
               0,                            // palette
               SpriteSize_16x16, SpriteColorFormat_256Color,
               sGfx, -1, false, false, false, false, false);
    }
}
```

### このパターンの利点

| | |
| --- | --- |
| メモリ | 起動時に確定。`malloc` 不要（7 章） |
| 速度 | 配列が連続しているのでキャッシュが効く |
| 安全性 | 上限を超えても「出ない」だけで壊れない |
| デバッグ | 配列全体をダンプすれば全状態が見える |

### OAM インデックスの割り当て

DS のスプライトは 128 個までです。`oamSet` の `id` が重複すると上書きし合うので、
**あらかじめ範囲を決めておく**とチーム開発で衝突しません。

```c
enum {
    OAM_PLAYER_BASE = 0,    // 0..3
    OAM_BULLET_BASE = 4,    // 4..35
    OAM_ENEMY_BASE  = 36,   // 36..67
};
```

---

## 10.3 ゲーム状態の管理 — `enum` + `switch`

タイトル画面、プレイ中、ポーズ、ゲームオーバー。これを状態機械にします。

```c
typedef enum {
    STATE_TITLE,
    STATE_PLAY,
    STATE_PAUSE,
    STATE_GAMEOVER,
} GameState;

static GameState sState = STATE_TITLE;

void gameUpdate(u32 held, u32 down)
{
    switch (sState) {
    case STATE_TITLE:
        if (down & KEY_START) {
            gameReset();
            sState = STATE_PLAY;
        }
        break;

    case STATE_PLAY:
        playerUpdate(held, down);
        bulletUpdate();
        enemyUpdate();
        checkCollisions();

        if (down & KEY_START)   sState = STATE_PAUSE;
        if (playerIsDead())     sState = STATE_GAMEOVER;
        break;

    case STATE_PAUSE:
        if (down & KEY_START) sState = STATE_PLAY;
        break;

    case STATE_GAMEOVER:
        if (down & KEY_A) sState = STATE_TITLE;
        break;
    }
}
```

`default:` を書かないでおくと、`enum` に状態を追加したときに
「この `switch` に対応が無い」と GCC が警告してくれます（6 章）。

### （発展）状態ごとの処理が長くなったら

関数ポインタのテーブルに逃がす手があります。合宿では `switch` で十分なことが多いです。

```c
typedef void (*UpdateFn)(u32 held, u32 down);

static const UpdateFn sUpdate[] = {
    [STATE_TITLE] = titleUpdate,
    [STATE_PLAY]  = playUpdate,
    [STATE_PAUSE] = pauseUpdate,
};

sUpdate[sState](held, down);
```

---

## 10.4 ファイル分割の設計

C には名前空間がありません。だから**ファイルを名前空間の代わりに使います**。

```
source/
  main.c        ← main() とゲームループだけ
  game.c/.h     ← 状態機械、全体の初期化
  player.c/.h   ← プレイヤー
  bullet.c/.h   ← 弾
  enemy.c/.h    ← 敵
  gfx.c/.h      ← VRAM 設定、スプライト読み込み
  sound.c/.h    ← maxmod のラッパ
  util.h        ← 共通のマクロと小関数（static inline）
```

BlocksDS のテンプレートは `SOURCEDIRS := source` なので、
**`source/` に置いたファイルは自動でビルドされます。** Makefile を触る必要はありません。

### 命名規則を決めておく

C には `Player::update()` が書けないので、**接頭辞で代用します**。

```c
void playerInit(void);
void playerUpdate(u32 keys);
void playerDraw(void);
int  playerGetHp(void);
```

- 公開関数: `モジュール名 + 動詞`（`playerUpdate`）
- ファイル内限定: `static` を付ける
- ファイルスコープ変数: `s` で始める（`sPlayerX`）
- グローバル変数: `g` で始める（`gFrameCount`）— そもそも減らす

合宿の初日にこのルールを共有しておくと、
「どの関数がどのファイルにあるか」が名前から分かるようになります。

### ヘッダに書くもの・書かないもの

| ヘッダ（`.h`）に書く | ソース（`.c`）に書く |
| --- | --- |
| 公開する関数のプロトタイプ宣言 | 関数の定義（本体） |
| 公開する型（`typedef struct`） | ファイル内限定の型 |
| `#define` の定数 | `static` 変数 |
| `static inline` の小さな関数 | `static` 関数 |
| `extern` 変数の宣言 | その変数の定義 |

**ヘッダに変数の定義を書かない**でください（`int gScore = 0;` を `.h` に書くと、
include したファイルの数だけ定義ができてリンクエラーになります）。

---

## 10.5 当たり判定の書き方

固定小数点と組み合わせた、よくあるパターンです。

```c
// 矩形同士（AABB）
static inline bool overlapRect(int ax, int ay, int aw, int ah,
                               int bx, int by, int bw, int bh)
{
    return ax < bx + bw && bx < ax + aw
        && ay < by + bh && by < ay + ah;
}

// 円同士（平方根を使わない！ 距離の 2 乗で比較する）
static inline bool overlapCircle(int ax, int ay, int ar,
                                 int bx, int by, int br)
{
    int dx = ax - bx;
    int dy = ay - by;
    int r  = ar + br;
    return dx * dx + dy * dy < r * r;
}
```

**`sqrt` を使わずに距離の 2 乗で比較する**のは、ゲームプログラミングの定番テクニックです。
DS には平方根器がありますが、それでも使わないに越したことはありません。

判定は**整数座標**（`>> 12` した後の値）で行うと、乗算のオーバーフローを避けられます。

```c
int px = player.x >> FP;
int py = player.y >> FP;
int ex = enemy.x  >> FP;
int ey = enemy.y  >> FP;

if (overlapCircle(px, py, 8, ex, ey, 8)) {
    playerDamage(1);
}
```

### 総当たり判定は N² に注意

```c
for (int i = 0; i < MAX_BULLETS; i++)
    for (int j = 0; j < MAX_ENEMIES; j++)
        ...
```

32 × 32 = 1024 回。これくらいなら DS でも 60fps で回りますが、
数が増えたら**先に `active` でスキップする**、
**大きく外れているものは早期に弾く**などの工夫を入れてください。

```c
for (int i = 0; i < MAX_BULLETS; i++) {
    if (!sBullets[i].active) continue;     // ← これだけでかなり違う
    ...
}
```

---

## 10.6 まとめ: 最小の動くゲーム

ここまでの型を全部使った、動く最小構成です。

```c
#include <nds.h>
#include <stdio.h>

#define FP 12
#define TO_FP(n)  ((n) << FP)
#define TO_INT(n) ((n) >> FP)

typedef struct {
    int x, y;       // 固定小数点
    int vx, vy;
} Player;

static Player sPlayer;

static void playerInit(void)
{
    sPlayer = (Player){ .x = TO_FP(128), .y = TO_FP(96) };
}

static void playerUpdate(u32 keys)
{
    const int SPEED = TO_FP(1) + TO_FP(1) / 2;   // 1.5 px/frame

    sPlayer.vx = 0;
    sPlayer.vy = 0;
    if (keys & KEY_LEFT)  sPlayer.vx = -SPEED;
    if (keys & KEY_RIGHT) sPlayer.vx =  SPEED;
    if (keys & KEY_UP)    sPlayer.vy = -SPEED;
    if (keys & KEY_DOWN)  sPlayer.vy =  SPEED;

    sPlayer.x += sPlayer.vx;
    sPlayer.y += sPlayer.vy;

    // 画面内に収める
    if (sPlayer.x < 0)             sPlayer.x = 0;
    if (sPlayer.x > TO_FP(255))    sPlayer.x = TO_FP(255);
    if (sPlayer.y < 0)             sPlayer.y = 0;
    if (sPlayer.y > TO_FP(191))    sPlayer.y = TO_FP(191);
}

int main(void)
{
    consoleDemoInit();
    playerInit();

    while (1)
    {
        scanKeys();
        u32 keys = keysHeld();

        if (keys & KEY_START) break;

        playerUpdate(keys);

        swiWaitForVBlank();

        consoleClear();
        printf("十字キー: 移動\n");
        printf("START:    終了\n\n");
        printf("x = %3d\n", TO_INT(sPlayer.x));
        printf("y = %3d\n", TO_INT(sPlayer.y));
    }

    return 0;
}
```

スプライトはまだ出していませんが、
**ゲームループ・入力・固定小数点・状態管理の型はすべて入っています。**
合宿ではここにグラフィックスを足していくことになります。

---

## 確認問題

**Q1.** ゲームループで「更新」と「描画」を分ける理由を 2 つ挙げてください。

<details><summary>解答</summary>

1. **VRAM / OAM への書き込みは VBlank 期間に行う必要がある。**
   画面を走査している最中に書き換えると、表示が崩れる（ティアリング）。
   更新（計算）はいつやってもよいが、描画（転送）はタイミングが決まっている。

2. **コードの見通しが良くなる。**
   ゲームロジックが「状態を変える」ことだけに専念できるので、
   テストしやすく、描画方法を変えてもロジックを触らずに済む。

</details>

**Q2.** 弾を管理するのに、リンクリストではなく固定長の構造体配列を使う理由を述べてください。

<details><summary>解答</summary>

- **`malloc` / `free` が不要** — DS はメモリが 4MB しかなく、
  実行中の動的確保はフラグメンテーションのリスクがある（7 章）
- **メモリ使用量が起動時に確定する** — 実行中に「足りない」が起きない
- **キャッシュ効率が良い** — 要素がメモリ上で連続しているので、
  全走査するゲームループと相性が良い。リンクリストはポインタを辿るたびにキャッシュミスする
- **上限を超えても壊れない** — 空きが無ければ弾が出ないだけ

C にはリストや辞書が標準で無く、自作するとバグの温床になるという実務的な理由もあります。
</details>

**Q3.** 円形の当たり判定で `sqrt` を使わない書き方と、その理由を説明してください。

<details><summary>解答</summary>

```c
// 距離の 2 乗どうしを比較する
int dx = ax - bx;
int dy = ay - by;
int r  = ar + br;
bool hit = (dx * dx + dy * dy) < (r * r);
```

`sqrt(dx*dx + dy*dy) < r` の両辺を 2 乗しても大小関係は変わらない（両辺とも非負なので）ため、
平方根を計算する必要がありません。

理由: 平方根は除算と並んで重い演算です。
DS にはハードウェア平方根器がありますが、それでもレジスタへの書き込みと待機が必要で、
乗算 2 回と加算 1 回に比べれば桁違いに遅くなります。
毎フレーム数百回まわる当たり判定では効いてきます。

なお、`dx * dx` はオーバーフローに注意してください。
固定小数点のまま計算せず、整数座標に落としてから判定するのが安全です。
</details>

---

[← 前章: 9. 固定小数点数](09-fixed-point.md) | [目次](../README.md) | [次章: 11. デバッグ →](11-debug.md)
