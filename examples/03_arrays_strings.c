// 第5章: 配列と文字列
// gcc -Wall -Wextra -o 03_arrays_strings 03_arrays_strings.c && ./03_arrays_strings
#include <stdio.h>
#include <string.h>

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

// 配列を受け取るとポインタになる → 長さは別途渡す
static void printAll(const int *arr, size_t n)
{
    for (size_t i = 0; i < n; i++) printf("%d ", arr[i]);
    printf("\n");
}

// 配列の「減衰」を実演
// ここは意図的に警告を出させている:
//   warning: 'sizeof' on array function parameter 'a' will return size of 'int *'
// GCC は「その sizeof は配列のサイズにならないぞ」と教えてくれる。
static void showDecay(int a[10])
{
    printf("  関数の中の sizeof(a)     = %zu  <- ポインタのサイズ\n", sizeof(a));
}

#define MAP_W 8
#define MAP_H 4
#define TILE_WALL 1

static unsigned char sMap[MAP_W * MAP_H];

// 境界チェックを関数の中に閉じ込める
static unsigned char mapGet(int x, int y)
{
    if (x < 0 || x >= MAP_W || y < 0 || y >= MAP_H) return TILE_WALL;
    return sMap[y * MAP_W + x];
}

int main(void)
{
    printf("=== 配列の減衰 ===\n");
    int arr[10] = {0};
    printf("  main の中の sizeof(arr) = %zu  <- 配列全体のサイズ\n", sizeof(arr));
    showDecay(arr);

    printf("\n=== a[i] == *(a+i) ===\n");
    int v[4] = {10, 20, 30, 40};
    printf("v[2]=%d  *(v+2)=%d  同じ\n", v[2], *(v + 2));
    printAll(v, ARRAY_SIZE(v));

    printf("\n=== 文字列は '\\0' 終端の char 配列 ===\n");
    char s[] = "abc";
    printf("sizeof(\"abc\") の配列 = %zu  (終端の '\\0' を含む)\n", sizeof(s));
    printf("strlen = %zu\n", strlen(s));
    for (size_t i = 0; i < sizeof(s); i++)
        printf("  s[%zu] = %3d ('%c')\n", i, s[i], s[i] ? s[i] : ' ');

    printf("\n=== 文字列の比較は strcmp ===\n");
    const char *p = "hello";
    char q[] = "hello";
    printf("p == q      -> %s  <- アドレス比較なので偽\n",
           ((const char *)q == p) ? "true" : "false");
    printf("strcmp == 0 -> %s  <- 内容比較\n",
           (strcmp(p, q) == 0) ? "true" : "false");

    printf("\n=== snprintf で安全に組み立てる ===\n");
    char buf[16];
    snprintf(buf, sizeof(buf), "SCORE: %d", 12345);
    printf("buf = \"%s\"\n", buf);
    // 意図的に溢れさせる（GCC が -Wformat-truncation で警告してくれる）
    snprintf(buf, sizeof(buf), "%s", "THIS-STRING-IS-WAY-TOO-LONG");
    printf("溢れても安全: \"%s\"  (%zu 文字で打ち切り + '\\0')\n",
           buf, strlen(buf));

    printf("\n=== 2次元マップと境界チェック ===\n");
    memset(sMap, 0, sizeof(sMap));
    sMap[1 * MAP_W + 2] = 7;
    printf("mapGet(2, 1)   = %d\n", mapGet(2, 1));
    printf("mapGet(-1, 0)  = %d  <- 画面外は壁扱い\n", mapGet(-1, 0));
    printf("mapGet(99, 99) = %d  <- クラッシュしない\n", mapGet(99, 99));

    return 0;
}
