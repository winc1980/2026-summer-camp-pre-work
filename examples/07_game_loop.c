// 第10章: ゲームコードの型（PC 上でシミュレート）
//
// DS では swiWaitForVBlank() / scanKeys() / oamSet() を使うところを、
// PC で動かせるように「ダミー入力」と「テキスト描画」に置き換えたもの。
// 構造そのものは DS でもそのまま使える。
//
// gcc -Wall -Wextra -o 07_game_loop 07_game_loop.c && ./07_game_loop
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#define FP        12
#define TO_FP(n)  ((n) << FP)
#define TO_INT(n) ((n) >> FP)

#define SCREEN_W 40
#define SCREEN_H 12

// --- 入力（DS の KEY_* に相当） -----------------------------------
enum {
    KEY_LEFT  = 1 << 0,
    KEY_RIGHT = 1 << 1,
    KEY_A     = 1 << 2,
    KEY_START = 1 << 3,
};

// --- プレイヤー ---------------------------------------------------
typedef struct { int x, y; } Player;
static Player sPlayer;

static void playerInit(void)
{
    sPlayer = (Player){ .x = TO_FP(SCREEN_W / 2), .y = TO_FP(SCREEN_H - 1) };
}

static void playerUpdate(uint32_t held)
{
    const int SPEED = TO_FP(1) + TO_FP(1) / 2;   // 1.5 /frame

    if (held & KEY_LEFT)  sPlayer.x -= SPEED;
    if (held & KEY_RIGHT) sPlayer.x += SPEED;

    if (sPlayer.x < 0)                  sPlayer.x = 0;
    if (sPlayer.x > TO_FP(SCREEN_W - 1)) sPlayer.x = TO_FP(SCREEN_W - 1);
}

// --- 弾（オブジェクトプール） -------------------------------------
#define MAX_BULLETS 8

typedef struct {
    bool active;
    int  x, y, vy;
} Bullet;

static Bullet sBullets[MAX_BULLETS];

static void bulletInit(void) { memset(sBullets, 0, sizeof(sBullets)); }

static void bulletSpawn(int x, int y)
{
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (sBullets[i].active) continue;
        sBullets[i] = (Bullet){ .active = true, .x = x, .y = y,
                                .vy = -TO_FP(1) };
        return;
    }
}

static void bulletUpdate(void)
{
    for (int i = 0; i < MAX_BULLETS; i++) {
        Bullet *b = &sBullets[i];
        if (!b->active) continue;
        b->y += b->vy;
        if (TO_INT(b->y) < 0) b->active = false;
    }
}

// --- 状態機械 -----------------------------------------------------
typedef enum { STATE_TITLE, STATE_PLAY, STATE_OVER } GameState;
static GameState sState = STATE_TITLE;
static int sFrame = 0;

static void gameReset(void)
{
    playerInit();
    bulletInit();
    sFrame = 0;
}

static void gameUpdate(uint32_t held, uint32_t down)
{
    switch (sState) {
    case STATE_TITLE:
        if (down & KEY_START) { gameReset(); sState = STATE_PLAY; }
        break;

    case STATE_PLAY:
        playerUpdate(held);
        bulletUpdate();
        if (down & KEY_A) bulletSpawn(sPlayer.x, sPlayer.y - TO_FP(1));
        if (++sFrame >= 24) sState = STATE_OVER;
        break;

    case STATE_OVER:
        break;
    }
}

// --- 描画（DS では oamSet に相当） --------------------------------
static void gameDraw(void)
{
    char fb[SCREEN_H][SCREEN_W + 1];
    for (int y = 0; y < SCREEN_H; y++) {
        memset(fb[y], '.', SCREEN_W);
        fb[y][SCREEN_W] = '\0';
    }

    for (int i = 0; i < MAX_BULLETS; i++) {
        const Bullet *b = &sBullets[i];
        if (!b->active) continue;
        int x = TO_INT(b->x), y = TO_INT(b->y);
        if (x >= 0 && x < SCREEN_W && y >= 0 && y < SCREEN_H) fb[y][x] = '|';
    }

    int px = TO_INT(sPlayer.x), py = TO_INT(sPlayer.y);
    if (px >= 0 && px < SCREEN_W && py >= 0 && py < SCREEN_H) fb[py][px] = 'A';

    printf("frame %2d  state=%s\n", sFrame,
           sState == STATE_TITLE ? "TITLE" :
           sState == STATE_PLAY  ? "PLAY"  : "OVER");
    for (int y = 0; y < SCREEN_H; y++) printf("  %s\n", fb[y]);
    printf("\n");
}

// --- 入力のダミー再生 ----------------------------------------------
// DS では scanKeys() / keysHeld() / keysDown() がこれを返す
static uint32_t fakeInput(int frame)
{
    if (frame == 0)               return KEY_START;
    if (frame >= 2  && frame < 8) return KEY_RIGHT | (frame % 3 == 0 ? KEY_A : 0);
    if (frame >= 8  && frame < 16) return KEY_LEFT | (frame % 4 == 0 ? KEY_A : 0);
    return 0;
}

int main(void)
{
    uint32_t prev = 0;

    for (int frame = 0; frame < 26; frame++)
    {
        // --- 1. 入力（DS: scanKeys / keysHeld / keysDown） ---
        uint32_t held = fakeInput(frame);
        uint32_t down = held & ~prev;      // 前フレームで押されていなかったもの
        prev = held;

        // --- 2. 更新（描画はしない） ---
        gameUpdate(held, down);

        // --- 3. DS ではここで swiWaitForVBlank() ---

        // --- 4. 描画 ---
        if (frame % 4 == 0 || sState == STATE_OVER) gameDraw();

        if (sState == STATE_OVER) break;
    }

    printf("GAME OVER\n");
    return 0;
}
