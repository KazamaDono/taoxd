/* AArch64 variant of the Ch10 leak+overflow lab target.
 *
 * Differences from the x86-64 build (`vuln.c`):
 *   - PIE is left ON.  The lab is meant to show a two-leak solve on arm64:
 *     one leak for libc (the dlsym-published pointer), and one leak for
 *     the executable's PIE base (a `main` self-address in the banner).
 *   - The overflow lives in the same shape (small stack buffer + linear
 *     `read()` past its end), but the return-address slot on arm64 is
 *     placed by the function prologue (`stp x29, x30, [sp, #N]!`), so the
 *     buffer-to-saved-LR offset is derived per build with `cyclic()`.
 *
 * Build with:
 *   aarch64-linux-gnu-gcc -O1 -Wall -Wno-format-security                 \
 *       -fno-stack-protector -D_GNU_SOURCE -o vuln-arm64 vuln-arm64.c
 *
 * Run under qemu-user (`qemu-aarch64 -L /usr/aarch64-linux-gnu ./vuln-arm64`)
 * as described in `../../lab/README.md`.  The solver for arm64 is left as
 * Exercise 6 in the chapter.
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dlfcn.h>

static void __attribute__((noinline)) banner(void) {
    void *libc_sym = dlsym(RTLD_DEFAULT, "puts");
    /* Two leaks: libc symbol AND our own `main` (PIE base). */
    printf("welcome. libc=%p main=%p\n", libc_sym, (void *)&main);
    fflush(stdout);
}

static void __attribute__((noinline)) handle(void) {
    char name[64];
    banner();
    printf("name? ");
    fflush(stdout);
    ssize_t n = read(0, name, 512);
    if (n <= 0) return;
    printf("hello, %.*s\n", (int)n, name);
    fflush(stdout);
}

int main(void) {
    setbuf(stdout, NULL);
    handle();
    return 0;
}
