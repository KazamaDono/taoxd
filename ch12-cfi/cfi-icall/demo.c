/* ch12-cfi/cfi-icall/demo.c
 * Build: clang -O1 -flto -fvisibility=hidden -fsanitize=cfi \
 *              -fno-sanitize-trap=cfi -fsanitize-recover=cfi \
 *              -o demo demo.c
 * Tested on Ubuntu 24.04, clang 18. */
#include <stdio.h>
#include <stdint.h>
#include <string.h>

/* Two functions with DIFFERENT signatures. */
static int add_ints(int a, int b) { return a + b; }                 /* ❶ */
static void print_str(const char *s) { puts(s); }

/* The call site expects int(*)(int,int). */
typedef int (*binop_t)(int, int);

int main(int argc, char **argv) {
    binop_t fp = add_ints;                                          /* ❷ */
    printf("legal call: %d\n", fp(2, 3));

    /* Overwrite the function pointer with a wrongly-typed target.
     * In a real bug this would be a heap or stack corruption; here
     * we just do it directly so the demo is self-contained. */
    memcpy(&fp, &(void *){ (void *)print_str }, sizeof fp);         /* ❸ */

    printf("about to make the illegal call...\n");
    int r = fp(2, 3);                                               /* ❹ */
    printf("survived: %d\n", r);
    return 0;
}
