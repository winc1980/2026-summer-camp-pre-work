// 第2章: 型と数値
// PC でビルド: gcc -Wall -Wextra -o 01_types 01_types.c && ./01_types
#include <stdio.h>
#include <stdint.h>

int main(void)
{
    printf("=== このマシンでの型のサイズ ===\n");
    printf("char       %zu\n", sizeof(char));
    printf("short      %zu\n", sizeof(short));
    printf("int        %zu\n", sizeof(int));
    printf("long       %zu   (DS では 4)\n", sizeof(long));
    printf("long long  %zu\n", sizeof(long long));
    printf("float      %zu\n", sizeof(float));
    printf("double     %zu\n", sizeof(double));
    printf("void *     %zu   (DS では 4)\n", sizeof(void *));

    printf("\n=== 整数昇格 ===\n");
    uint8_t a = 200, b = 100;
    uint8_t sum8  = a + b;      // int で 300 → u8 に切り詰め
    int     sum32 = a + b;
    printf("u8  a+b = %d  (300 が切り詰められる)\n", sum8);
    printf("int a+b = %d\n", sum32);

    printf("\n=== 符号あり/なしの比較 ===\n");
    // ここは意図的に警告を出させている:
    //   warning: comparison of integer expressions of different signedness
    // -Wall/-Wextra はこの罠をちゃんと教えてくれる、という実演。
    int      i = -1;
    unsigned u = 1;
    printf("(-1 < 1u) = %s  <- -1 が巨大な符号なし値に変換される\n",
           (i < u) ? "true" : "false");

    printf("\n=== 符号なしのラップアラウンド ===\n");
    uint32_t n = 3;
    printf("u32 3 - 5 = %u\n", n - 5);
    uint16_t m = 3;
    printf("u16 3 - 5 = %d  <- int に昇格するので負のまま\n", m - 5);

    printf("\n=== 整数除算 ===\n");
    int hp = 37, maxHp = 100;
    printf("hp / maxHp * 100 = %d   <- 常に 0\n", hp / maxHp * 100);
    printf("hp * 100 / maxHp = %d   <- 正しい\n", hp * 100 / maxHp);
    printf(" 7 / 2 = %d,  -7 / 2 = %d  (0 方向に切り捨て)\n", 7 / 2, -7 / 2);
    printf(" 7 %% 2 = %d,  -7 %% 2 = %d\n", 7 % 2, -7 % 2);

    printf("\n=== char の符号 ===\n");
    char c = 127;
    c++;
    printf("char 127+1 = %d  (x86 では -128、ARM では 128)\n", c);

    return 0;
}
