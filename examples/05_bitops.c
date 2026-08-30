// 第8章: ビット演算とハードウェアレジスタ
// gcc -Wall -Wextra -o 05_bitops 05_bitops.c && ./05_bitops
#include <stdio.h>
#include <stdint.h>

#define BIT(n) (1u << (n))

static void printBits8(const char *label, uint8_t v)
{
    printf("%-11s ", label);
    for (int i = 7; i >= 0; i--) printf("%d", (v >> i) & 1);
    printf("  (0x%02X)\n", v);
}

// DS の色: BGR555
static inline uint16_t myRGB15(int r, int g, int b)
{
    return (uint16_t)((r & 31) | ((g & 31) << 5) | ((b & 31) << 10));
}
static inline int getR(uint16_t c) { return  c        & 31; }
static inline int getG(uint16_t c) { return (c >>  5) & 31; }
static inline int getB(uint16_t c) { return (c >> 10) & 31; }

int main(void)
{
    printf("=== 4 つの基本イディオム ===\n");
    uint8_t flags = 0b10110011;
    printBits8("original", flags);

    flags |= BIT(3);
    printBits8("|=  BIT(3)", flags);

    flags &= (uint8_t)~BIT(3);
    printBits8("&= ~BIT(3)", flags);

    flags ^= BIT(7);
    printBits8("^=  BIT(7)", flags);

    printf("BIT(5) は立っている? %s\n", (flags & BIT(5)) ? "yes" : "no");

    printf("\n=== マスクでフィールドを取り出す ===\n");
    uint8_t v = 0xB7;   // 0b10110111
    printBits8("value", v);
    printf("下位 6bit (color) = %u\n",  v & 0x3F);
    printf("上位 2bit (prio)  = %u\n", (v >> 6) & 0x03);

    printf("\n=== 優先順位の罠 ===\n");
    unsigned keys = BIT(0);   // KEY_A 相当
    printf("keys & BIT(0) == 0  -> %d  (誤り: 常に 0)\n", keys & (BIT(0) == 0));
    printf("(keys & BIT(0)) == 0 -> %d  (正しい)\n", (keys & BIT(0)) == 0);

    printf("\n=== DS の色 (BGR555) ===\n");
    uint16_t red    = myRGB15(31,  0,  0);
    uint16_t green  = myRGB15( 0, 31,  0);
    uint16_t blue   = myRGB15( 0,  0, 31);
    uint16_t rg     = myRGB15(31, 15,  0);
    printf("red    = 0x%04X\n", red);
    printf("green  = 0x%04X\n", green);
    printf("blue   = 0x%04X\n", blue);
    printf("R31G15 = 0x%04X  (8 章 確認問題 Q3 の答え)\n", rg);
    printf("分解: R=%d G=%d B=%d\n", getR(rg), getG(rg), getB(rg));

    printf("\n=== シフトは 2 のべき乗の乗除 ===\n");
    int x = 100;
    printf("100 << 3 = %d  (= 100 * 8)\n", x << 3);
    printf("100 >> 2 = %d  (= 100 / 4)\n", x >> 2);

    printf("\n=== 便利なビット技 ===\n");
    for (unsigned n = 1; n <= 16; n <<= 1)
        printf("  %2u は 2 のべき乗? %s\n", n, (n && !(n & (n - 1))) ? "yes" : "no");
    printf("  %2u は 2 のべき乗? %s\n", 12u, (12u && !(12u & 11u)) ? "yes" : "no");
    printf("popcount(0xB7) = %d\n", __builtin_popcount(0xB7));
    unsigned size = 13;
    printf("13 を 4 の倍数に切り上げ = %u\n", (size + 3) & ~3u);

    return 0;
}
