// 第4章: ポインタ
// gcc -Wall -Wextra -o 02_pointers 02_pointers.c && ./02_pointers
#include <stdio.h>

// out 引数: 呼び出し元の変数を書き換える
static void damage(int *hp, int amount)
{
    *hp -= amount;
    if (*hp < 0) *hp = 0;
}

static void swap(int *a, int *b)
{
    int t = *a;
    *a = *b;
    *b = t;
}

// 複数の値を返す
static void getCenter(int *x, int *y)
{
    *x = 128;
    *y = 96;
}

int main(void)
{
    printf("=== & と * ===\n");
    int  x = 42;
    int *p = &x;
    printf("x  = %d\n", x);
    printf("&x = %p\n", (void *)&x);
    printf("p  = %p   <- p は x のアドレスを持つ\n", (void *)p);
    printf("*p = %d   <- p が指す先の値\n", *p);
    *p = 100;
    printf("*p = 100 した後の x = %d\n", x);

    printf("\n=== 値渡し vs ポインタ渡し ===\n");
    int hp = 10;
    damage(&hp, 3);
    printf("damage(&hp, 3) 後の hp = %d\n", hp);

    int a = 1, b = 2;
    swap(&a, &b);
    printf("swap 後: a=%d b=%d\n", a, b);

    int cx, cy;
    getCenter(&cx, &cy);
    printf("center = (%d, %d)\n", cx, cy);

    printf("\n=== ポインタ演算（型のサイズ単位で進む）===\n");
    int arr[4] = {10, 20, 30, 40};
    int *q = arr;
    for (int i = 0; i < 4; i++) {
        printf("  q+%d -> addr %p  value %d\n", i, (void *)(q + i), *(q + i));
    }
    printf("アドレスの差は %d バイト (= sizeof(int))\n",
           (int)((char *)(q + 1) - (char *)q));

    printf("\n=== 練習: ポインタを追う ===\n");
    int m = 1, n = 2;
    int *pm = &m, *pn = &n;
    *pm = *pn;    // (1) m に n の値を代入
    pm  = pn;     // (2) pm が n を指すようになる
    *pm = 10;     // (3) n に 10 を代入
    printf("m=%d n=%d   (期待: m=2 n=10)\n", m, n);

    return 0;
}
