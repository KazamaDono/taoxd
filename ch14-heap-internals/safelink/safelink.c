/* build: gcc -O0 -g -no-pie safelink.c -o safelink   (glibc 2.39) */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#define REVEAL(pos, val) ((uintptr_t)(val) ^ ((uintptr_t)(pos) >> 12))     // ❶

int main(void) {
    void *a = malloc(0x30), *b = malloc(0x30), *c = malloc(0x30);
    malloc(0x30);                                    /* barrier: not freed */  // ❷
    free(a); free(b); free(c);                       /* tcache: c -> b -> a */

    uintptr_t raw_c = *(uintptr_t*)c;                /* mangled next in c */   // ❸
    uintptr_t raw_b = *(uintptr_t*)b;
    uintptr_t raw_a = *(uintptr_t*)a;

    printf("c=%p  raw c->next=0x%016lx  reveal=%p  (expect b=%p)\n",
           c, raw_c, (void*)REVEAL(c, raw_c), b);                              // ❹
    printf("b=%p  raw b->next=0x%016lx  reveal=%p  (expect a=%p)\n",
           b, raw_b, (void*)REVEAL(b, raw_b), a);
    printf("a=%p  raw a->next=0x%016lx  reveal=%p  (expect NULL)\n",
           a, raw_a, (void*)REVEAL(a, raw_a));

    printf("heap base leak from tail: 0x%lx (== a>>12: 0x%lx)\n",              // ❺
           raw_a, (uintptr_t)a >> 12);
    return 0;
}
