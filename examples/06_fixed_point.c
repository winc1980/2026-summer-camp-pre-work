// 第9章: 固定小数点数
// gcc -Wall -Wextra -o 06_fixed_point 06_fixed_point.c -lm && ./06_fixed_point
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#define FP        12
#define FP_ONE    (1 << FP)              // 4096 = 1.0
#define TO_FP(n)  ((n) << FP)
#define TO_INT(n) ((n) >> FP)
#define FLOAT_TO_FP(f) ((int)((f) * FP_ONE))

// 表示用: 固定小数点を "整数.小数" で出す（float を使わずに）
static void printFp(const char *label, int v)
{
    int sign = v < 0 ? -1 : 1;
    int a    = v * sign;
    int ip   = a >> FP;
    int fp   = (int)(((int64_t)(a & (FP_ONE - 1)) * 10000) >> FP);
    printf("%-16s %11d  = %s%d.%04d\n", label, v, sign < 0 ? "-" : "", ip, fp);
}

static inline int fpMul(int a, int b) { return (int)(((int64_t)a * b) >> FP); }
static inline int fpDiv(int a, int b) { return (int)(((int64_t)a << FP) / b); }

int main(void)
{
    printf("=== 変換 ===\n");
    printFp("1.0",  TO_FP(1));
    printFp("0.5",  FLOAT_TO_FP(0.5));
    printFp("1.25", FLOAT_TO_FP(1.25));
    printFp("-2.5", FLOAT_TO_FP(-2.5));
    printf("TO_INT(FLOAT_TO_FP(3.9)) = %d  <- 切り捨て\n",
           TO_INT(FLOAT_TO_FP(3.9)));

    printf("\n=== 加算はそのまま ===\n");
    int a = TO_FP(3), b = TO_FP(2);
    printFp("3.0 + 2.0", a + b);

    printf("\n=== 乗算は >> 12 が必要 ===\n");
    printf("a * b          = %11d  <- 4096 倍ずれている\n", a * b);
    printFp("(a*b) >> 12", (a * b) >> FP);
    printFp("fpMul(a, b)",  fpMul(a, b));

    printf("\n=== オーバーフローに注意 ===\n");
    int big = TO_FP(100);            // 100.0 -> 409600
    printf("big * big は %lld。int(32bit) には入らない\n",
           (long long)big * big);
    printf("(big*big) >> 12 = %d  <- 溢れて壊れた値\n", (big * big) >> FP);
    printFp("fpMul(big,big)", fpMul(big, big));   // 64bit 経由なら正しい

    // ただし 64bit を経由しても、結果自体が 12bit 固定小数点の
    // 表現範囲(約 +-524288)に収まらなければ意味がない
    int huge = TO_FP(1000);
    printf("fpMul(1000.0, 1000.0) = %d  <- 1,000,000 は表現範囲外なので壊れる\n",
           fpMul(huge, huge));

    printf("\n=== 除算は << してから割る ===\n");
    printFp("3.0 / 2.0", fpDiv(TO_FP(3), TO_FP(2)));
    printFp("1.0 / 3.0", fpDiv(TO_FP(1), TO_FP(3)));

    printf("\n=== 0.5px ずつ動かす（整数座標では不可能）===\n");
    int x  = TO_FP(100);
    int vx = FP_ONE / 2;          // 0.5
    printf("frame:  ");
    for (int f = 0; f < 8; f++) { printf("%4d", TO_INT(x)); x += vx; }
    printf("\n");

    printf("\n=== 重力つきの弾道 ===\n");
    #define GRAVITY (FP_ONE / 16)     // 0.0625 px/frame^2
    int by = TO_FP(180), bvy = -FLOAT_TO_FP(3.0);
    for (int f = 0; f < 12; f++) {
        bvy += GRAVITY;
        by  += bvy;
        printf("  frame %2d: y = %3d\n", f, TO_INT(by));
    }

    printf("\n=== 摩擦（2 のべき乗ならシフトだけ）===\n");
    int v = TO_FP(20);
    printf("v:  ");
    for (int f = 0; f < 8; f++) { printf("%6d", TO_INT(v * 100) ); v -= v >> 3; }
    printf("   (x100 で表示。毎フレーム 1/8 減衰)\n");

    printf("\n=== 円の当たり判定は距離の2乗で（sqrt 不要）===\n");
    int ax = 100, ay = 100, ar = 8;
    int bx2 = 110, by2 = 103, br = 8;
    int dx = ax - bx2, dy = ay - by2, r = ar + br;
    printf("dx=%d dy=%d  dx^2+dy^2=%d  r^2=%d  -> %s\n",
           dx, dy, dx*dx + dy*dy, r*r,
           (dx*dx + dy*dy < r*r) ? "HIT" : "MISS");

    return 0;
}
