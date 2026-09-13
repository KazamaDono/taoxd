/* ch10-aslr/partial/vuln.c
 *
 * A minimal one-byte partial-overwrite lab.
 *
 * The vulnerable program stores a callback pointer immediately after a
 * fixed-size input buffer and lets a `read()` walk one byte past that
 * buffer, straight into the LOW byte of the pointer.  Because the
 * low 12 bits of any address inside `.text` are frozen by the ELF layout
 * (only ASLR's high bits move), rewriting the low byte redirects the
 * indirect call to another function in the same 256-byte .text window
 * WITHOUT knowing the region's base -- i.e. without a leak.
 *
 * The technique generalises directly to a saved return address on a
 * fork-serving target (Exercise 4 in the chapter): the demonstration
 * shape is the same, only the location of the pointer differs.
 *
 * Build (see Makefile for the annotated flags):
 *   gcc -O0 -fno-stack-protector -no-pie -D_GNU_SOURCE
 *       -fno-inline -o vuln vuln.c
 *
 * NOTE: `-no-pie` gives us fixed, discoverable symbol addresses for
 * `default_cb` and `win`, so the layout requirement (they live in the
 * same 256-byte .text window) is enforced by the Makefile's `layout`
 * check target rather than by chance.
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef void (*cb_t)(void);

static void __attribute__((noinline)) default_cb(void) {
    puts("[default] wrong byte; bailing.");
    fflush(stdout);
    _exit(0);
}

/* Deliberately placed adjacent to `default_cb`; the Makefile asserts
 * that `(win & ~0xff) == (default_cb & ~0xff)` after link. */
static void __attribute__((noinline)) win(void) {
    puts("WIN");
    fflush(stdout);
    /* Instead of spawning /bin/sh (which drags in a full pty for the
     * pytest harness), emit a distinctive marker the CI test greps. */
    puts("CH10_PARTIAL_LANDED_9f2c8a");
    fflush(stdout);
    _exit(0);
}

/* Layout under our control: buffer directly followed by the callback
 * pointer, no padding.  The one-byte overrun of `read()` lands on the
 * low byte of `S.cb`. */
struct pack {
    char buf[64];
    cb_t cb;
} S;

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
    S.cb = default_cb;
    puts("input?");
    fflush(stdout);
    /* Off-by-one: buffer is 64 bytes; we permit read of 65. */
    ssize_t n = read(0, S.buf, 65);
    if (n <= 0) return 1;
    /* Invoke through the (possibly-corrupted) callback pointer. */
    S.cb();
    return 0;
}
