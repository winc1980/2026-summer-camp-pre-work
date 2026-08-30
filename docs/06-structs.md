# 6. 構造体と列挙型

> **この章で分かること**
>
> - `typedef struct` のイディオムを書ける
> - `.` と `->` を使い分けられる
> - パディングとアライメントを理解する
> - `enum` でゲームの状態やフラグを表現できる
> - 構造体配列でゲームのデータを持つ

---

## 6.1 構造体の基本

複数の値をひとまとめにした型を作ります。オブジェクト指向言語のクラスから
メソッドと継承を抜いたもの、と思ってください。

```c
struct Player {
    int x, y;
    int hp;
    u8  lives;
};

struct Player p;      // 使うたびに struct と書く必要がある
p.x = 128;
```

### `typedef` を付けるのが定番

`struct` と毎回書くのは面倒なので、C では `typedef` して短い名前を付けます。

```c
typedef struct {
    int x, y;
    int hp;
    u8  lives;
} Player;

Player p;             // すっきり
p.x = 128;
```

自己参照（自分自身へのポインタをメンバに持つ）場合はタグ名が必要です。

```c
typedef struct Node {          // ← タグ名 Node
    int          value;
    struct Node *next;         // 中では struct Node と書く
} Node;
```

libnds もこのスタイルです。

```c
typedef struct touchPosition {
    u16 rawx, rawy;
    u16 px, py;
    u16 z1, z2;
} touchPosition;
```

---

## 6.2 初期化

```c
Player p = {128, 96, 3, 2};             // メンバの順番通り
Player p = {0};                         // 全部 0（定番）

// 指定初期化子（C99）。こちらを推奨
Player p = {
    .x     = 128,
    .y     = 96,
    .hp    = 3,
    .lives = 2,
};
```

**指定初期化子を使ってください。**
メンバの順番を後から入れ替えても壊れませんし、書き忘れたメンバは 0 になります。
何より読んで意味が分かります。

宣言と初期化を分けるときは `memset` か、構造体リテラルの代入を使います。

```c
Player p;
memset(&p, 0, sizeof(p));               // 全部 0

p = (Player){ .x = 128, .y = 96 };      // 複合リテラル（C99）。これも便利
```

---

## 6.3 `.` と `->`

```c
Player  p;
Player *pp = &p;

p.x        = 10;      // 実体なら .
(*pp).x    = 10;      // ポインタなら間接参照してから .
pp->x      = 10;      // ↑ の省略記法。こちらを使う
```

**`->` は `(*p).` の糖衣構文**です。それだけです。

```c
void playerUpdate(Player *p)      // ポインタで受け取るので
{
    p->x += 1;                    // 中では -> を使う
    p->hp--;
}
```

DS のコードは構造体をポインタで渡すことが多いので、`->` が主役になります。

---

## 6.4 構造体は値としてコピーされる（配列と違う！）

**ここが配列との大きな違いです。**

```c
Player a = { .x = 1, .y = 2 };
Player b = a;              // 中身が丸ごとコピーされる

b.x = 99;
printf("%d\n", a.x);       // 1。a は変わらない
```

```c
int arr1[3] = {1,2,3};
int arr2 = arr1;           // コンパイルエラー。配列は代入できない
```

配列は代入もコピーもできないのに、構造体はできる。この非対称性は C の癖です。
（配列を構造体に入れると、構造体ごとコピーできるようになる、という裏技的な使い方もあります）

関数の引数も同様で、構造体は**丸ごとコピーされます**。

```c
void f(Player p);        // 構造体のサイズぶんコピーされる
void g(Player *p);       // 4 バイト（アドレス）だけ
void h(const Player *p); // 読むだけならこれ
```

小さい構造体（`Point{int x,y}` など）なら値渡しでも問題ありませんが、
**DS では原則ポインタ渡し**にしておくのが安全です。

---

## 6.5 構造体配列 — ゲームの基本データ構造

C にはリストも辞書もありません。ゲームのエンティティ管理は
**「固定サイズの構造体配列 + 有効フラグ」** が定番です。

```c
#define MAX_BULLETS 64

typedef struct {
    bool active;
    int  x, y;
    int  vx, vy;
} Bullet;

static Bullet sBullets[MAX_BULLETS];

// 空きスロットを探して弾を出す
void bulletSpawn(int x, int y, int vx, int vy)
{
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!sBullets[i].active) {
            sBullets[i] = (Bullet){
                .active = true,
                .x = x, .y = y, .vx = vx, .vy = vy,
            };
            return;
        }
    }
    // 空きが無ければ何もしない（弾が出ないだけ。落ちない）
}

void bulletUpdateAll(void)
{
    for (int i = 0; i < MAX_BULLETS; i++) {
        Bullet *b = &sBullets[i];      // ポインタを取っておくと読みやすい
        if (!b->active) continue;

        b->x += b->vx;
        b->y += b->vy;

        if (b->y < 0 || b->y > 192) b->active = false;
    }
}
```

このパターンの良いところ:

- **`malloc` / `free` が一切要らない**（フラグメンテーションが起きない）
- メモリ使用量が起動時に確定する。DS のように 4MB しかない環境では極めて重要
- 配列が連続しているのでキャッシュに乗りやすく、速い

**合宿ではこの形をベースにしてください。** 10 章でもう一度出てきます。

---

## 6.6 パディングとアライメント

構造体のサイズは、メンバのサイズの合計と一致するとは限りません。

```c
typedef struct {
    u8  a;      // 1 byte
    u32 b;      // 4 byte
    u8  c;      // 1 byte
} Bad;

printf("%zu\n", sizeof(Bad));   // 12（1+4+1 = 6 ではない）
```

なぜか。CPU は「4 バイトの値は 4 の倍数アドレスに置く」ことを要求（または強く選好）します。
これを **アライメント** と言い、コンパイラは条件を満たすために**詰め物（パディング）**を入れます。

```
オフセット:  0    1    2    3    4    5    6    7    8    9   10   11
           ┌────┬────┬────┬────┬────┬────┬────┬────┬────┬────┬────┬────┐
   Bad:    │ a  │pad │pad │pad │      b (4 byte)   │ c  │pad │pad │pad │
           └────┴────┴────┴────┴────┴────┴────┴────┴────┴────┴────┴────┘
```

メンバを**大きい順に並べ替える**と詰まります。

```c
typedef struct {
    u32 b;      // 4
    u8  a;      // 1
    u8  c;      // 1
                // pad 2
} Good;

printf("%zu\n", sizeof(Good));   // 8
```

DS のメインメモリは 4MB です。敵構造体を 1000 個持つなら、
1 個 4 バイトの差が 4KB の差になります。**大きいメンバから順に並べる**習慣を付けてください。

### ARM ではアライメント違反が実害になる

x86 は非アライメントアクセスを黙って処理してくれますが、
**ARM は違います**。奇数アドレスから `u16` を読もうとすると、
CPU が勝手にアドレスを丸めて**まったく別の値を返します**（例外にすらならないことがある）。

```c
u8  buf[10];
u16 *p = (u16 *)(buf + 1);    // 奇数アドレス
u16  v = *p;                  // ARM では壊れた値が返る
```

バイト列を解析するときにやりがちです。安全にやるなら `memcpy` を使います。

```c
u16 v;
memcpy(&v, buf + 1, sizeof(v));    // アライメントに関係なく正しく動く
```

---

## 6.7 列挙型（`enum`）

名前の付いた整数定数をまとめて定義します。

```c
typedef enum {
    STATE_TITLE,      // 0
    STATE_PLAY,       // 1
    STATE_PAUSE,      // 2
    STATE_GAMEOVER,   // 3
    STATE_COUNT,      // 4  ← 「個数」を取れる定番テクニック
} GameState;

GameState state = STATE_TITLE;
```

値を明示することもできます。

```c
typedef enum {
    DIR_UP    = 0,
    DIR_DOWN  = 1,
    DIR_LEFT  = 2,
    DIR_RIGHT = 3,
} Direction;
```

### `switch` と組み合わせる

`enum` を `switch` すると、**GCC が「case の書き漏れ」を警告してくれます**
（`-Wswitch`、`-Wall` に含まれる）。これは地味ですが非常に強力です。

```c
switch (state) {
case STATE_TITLE:    updateTitle();    break;
case STATE_PLAY:     updatePlay();     break;
case STATE_PAUSE:    updatePause();    break;
// STATE_GAMEOVER を書き忘れると warning が出る
}
```

`default:` を書いてしまうとこの警告が消えるので、
**`enum` を網羅する `switch` では `default` を書かない**という流儀もあります。

### ビットフラグとしての `enum`

```c
typedef enum {
    FLAG_INVINCIBLE = 1 << 0,   // 0x01
    FLAG_UNDERWATER = 1 << 1,   // 0x02
    FLAG_STUNNED    = 1 << 2,   // 0x04
    FLAG_POISONED   = 1 << 3,   // 0x08
} PlayerFlags;

u32 flags = 0;
flags |= FLAG_INVINCIBLE;              // 立てる
flags &= ~FLAG_STUNNED;                // 下ろす
if (flags & FLAG_POISONED) { ... }     // 判定
```

libnds のキー入力もまさにこれです（8 章で詳しく）。

```c
if (keysHeld() & KEY_A) { ... }
```

> C の `enum` は中身はただの `int` です。`GameState s = 99;` もエラーになりません。
> 「名前が付いた定数の集合」くらいの認識でいてください。

---

## 確認問題

**Q1.** `p.x` と `p->x` はいつどちらを使いますか。

<details><summary>解答</summary>

- `p` が構造体の実体なら `p.x`
- `p` が構造体へのポインタなら `p->x`（`(*p).x` の省略形）

関数の引数で構造体を受け取るときは大抵ポインタなので、
関数の中では `->` を使うことになります。
</details>

**Q2.** 次の構造体の `sizeof` を減らすには、どう書き換えますか。理由も述べてください。

```c
typedef struct {
    u8  hp;
    u32 score;
    u8  lives;
    u16 x;
} Stats;
```

<details><summary>解答</summary>

現状のレイアウト（4 バイト境界に揃えるためのパディング入り）:

```
hp(1) pad(3) score(4) lives(1) pad(1) x(2)  → 12 バイト
```

大きいメンバから順に並べ替えます:

```c
typedef struct {
    u32 score;   // 4
    u16 x;       // 2
    u8  hp;      // 1
    u8  lives;   // 1
} Stats;         // → 8 バイト。パディングなし
```

理由: CPU は N バイトの型を N の倍数アドレスに置くことを要求するため、
小さいメンバの後に大きいメンバが来ると隙間（パディング）が生まれます。
大きい順に並べれば隙間が最小になります。
</details>

**Q3.** 弾を最大 64 発まで管理する仕組みを、`malloc` を使わずに実装する方針を述べてください。

<details><summary>解答</summary>

固定長の構造体配列と `active` フラグによる「オブジェクトプール」にします。

```c
#define MAX_BULLETS 64
static Bullet sBullets[MAX_BULLETS];
```

- 生成: 配列を線形に走査して `active == false` のスロットを探し、そこに書き込む。
  空きが無ければ何もしない（弾が出ないだけで、クラッシュしない）
- 破棄: `active = false` にするだけ
- 更新/描画: 配列を全走査して `active` のものだけ処理

利点:

- 動的確保が無いのでフラグメンテーションも確保失敗も起きない
- 使用メモリが起動時に確定する（4MB しかない DS では決定的に重要）
- 配列が連続しているのでキャッシュ効率が良い

</details>

---

[← 前章: 5. 配列と文字列](05-arrays-strings.md) | [目次](../README.md) | [次章: 7. メモリ管理 →](07-memory.md)
