/* Lab target for Ch10. Build with:                                    */
/*   gcc -O1 -Wall -Wno-format-security -fno-stack-protector           */
/*       -no-pie -D_GNU_SOURCE                                         */
/*       -o vuln vuln.c                                                */
/* We disable the stack canary (Ch11) and PIE (Ch10 pattern), but      */
/* leave NX and ASLR on. Both flags are commented in the Makefile.     */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dlfcn.h>

static void __attribute__((noinline)) banner(void) {
    void *p = dlsym(RTLD_DEFAULT, "puts");         /* ❶ libc symbol addr */
    printf("welcome. debug=%p\n", p);
    fflush(stdout);
}

static void __attribute__((noinline)) handle(void) {
    char name[64];
    banner();                                      /* ❷ leak happens here */
    printf("name? ");
    fflush(stdout);
    /* Intentional: read up to 512 bytes into a 64-byte buffer.        */
    ssize_t n = read(0, name, 512);                /* ❸ the overflow    */
    if (n <= 0) return;
    printf("hello, %.*s\n", (int)n, name);
    fflush(stdout);
}

int main(void) {
    setbuf(stdout, NULL);
    handle();
    return 0;
}
