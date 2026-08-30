// 第6章: 構造体・共用体・列挙型
// gcc -Wall -Wextra -o 04_structs 04_structs.c && ./04_structs
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

// --- パディングの実演 --------------------------------------------
typedef struct {
    uint8_t  a;
    uint32_t b;
    uint8_t  c;
} Bad;

typedef struct {
    uint32_t b;
    uint8_t  a;
    uint8_t  c;
} Good;

// --- オブジェクトプール ------------------------------------------
#define MAX_BULLETS 8

typedef struct {
    bool active;
    int  x, y;
    int  vx, vy;
} Bullet;

static Bullet sBullets[MAX_BULLETS];

static void bulletSpawn(int x, int y, int vx, int vy)
{
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (sBullets[i].active) continue;
        sBullets[i] = (Bullet){
            .active = true, .x = x, .y = y, .vx = vx, .vy = vy,
        };
        return;
    }
    printf("  (空きスロットが無いので弾は出ない。クラッシュはしない)\n");
}

static void bulletUpdate(void)
{
    for (int i = 0; i < MAX_BULLETS; i++) {
        Bullet *b = &sBullets[i];
        if (!b->active) continue;
        b->x += b->vx;
        b->y += b->vy;
        if (b->y < 0 || b->y > 192) b->active = false;
    }
}

static int bulletCountActive(void)
{
    int n = 0;
    for (int i = 0; i < MAX_BULLETS; i++) if (sBullets[i].active) n++;
    return n;
}

// --- enum ---------------------------------------------------------
typedef enum {
    STATE_TITLE, STATE_PLAY, STATE_PAUSE, STATE_GAMEOVER, STATE_COUNT
} GameState;

static const char *stateName(GameState s)
{
    switch (s) {
    case STATE_TITLE:    return "TITLE";
    case STATE_PLAY:     return "PLAY";
    case STATE_PAUSE:    return "PAUSE";
    case STATE_GAMEOVER: return "GAMEOVER";
    case STATE_COUNT:    break;
    }
    return "?";
}

int main(void)
{
    printf("=== パディング ===\n");
    printf("Bad  (u8, u32, u8) sizeof = %zu\n", sizeof(Bad));
    printf("Good (u32, u8, u8) sizeof = %zu  <- 大きい順に並べる\n", sizeof(Good));

    printf("\n=== 構造体は値でコピーされる ===\n");
    Good g1 = { .b = 1, .a = 2, .c = 3 };
    Good g2 = g1;
    g2.b = 99;
    printf("g1.b = %u, g2.b = %u  <- 独立している\n", g1.b, g2.b);

    printf("\n=== . と -> ===\n");
    Good *pg = &g1;
    printf("g1.b = %u, pg->b = %u, (*pg).b = %u\n", g1.b, pg->b, (*pg).b);

    printf("\n=== オブジェクトプール ===\n");
    memset(sBullets, 0, sizeof(sBullets));
    for (int i = 0; i < 10; i++) {
        printf("spawn %d: ", i);
        bulletSpawn(128, 96, 0, -4);
        printf("active = %d\n", bulletCountActive());
    }

    printf("\n弾を更新して画面外に出す:\n");
    for (int frame = 0; frame < 30; frame++) {
        bulletUpdate();
        if (frame % 10 == 9)
            printf("  frame %2d: active = %d\n", frame + 1, bulletCountActive());
    }

    printf("\n=== enum + switch ===\n");
    for (GameState s = STATE_TITLE; s < STATE_COUNT; s++)
        printf("  %d = %s\n", s, stateName(s));

    printf("\n=== ビットフラグとしての enum ===\n");
    enum {
        FLAG_INVINCIBLE = 1 << 0,
        FLAG_UNDERWATER = 1 << 1,
        FLAG_STUNNED    = 1 << 2,
    };
    unsigned flags = 0;
    flags |= FLAG_INVINCIBLE;
    flags |= FLAG_STUNNED;
    printf("flags = 0x%02X\n", flags);
    flags &= ~FLAG_STUNNED;
    printf("STUNNED を下ろすと 0x%02X\n", flags);
    printf("INVINCIBLE? %s\n", (flags & FLAG_INVINCIBLE) ? "yes" : "no");

    return 0;
}
