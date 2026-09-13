/* Build: gcc -O2 -fcf-protection=full ibt_demo.c -o ibt_demo */
#include <stdio.h>

__attribute__((noinline)) int target(int x) { return x + 1; }   /* ❶ */

int main(void) {
    int (*fp)(int) = (int (*)(int))((char *)&target + 4);       /* ❷ */
    printf("indirect call skipping ENDBR64...\n");
    return fp(41);                                              /* ❸ */
}
