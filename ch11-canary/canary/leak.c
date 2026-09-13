/* ch11-canary/canary/leak.c
 *
 * Deliberately vulnerable single-connection service (stdin/stdout so pwntools
 * can drive it as a subprocess) used by Chapter 11 to demonstrate a canary
 * leak followed by a linear stack overflow across the reproduced canary.
 *
 * The bug shape:
 *   1. printf(user_fmt) reads the caller's stack -- the same frame that
 *      holds the canary, giving us a positional %N$p oracle.
 *   2. A raw read() overwrites the same buffer with more bytes than it
 *      holds, letting us cross the canary and rewrite the saved return
 *      address underneath it.
 *
 * Frame layout: char buf[128] plus a long pad, so buf-to-canary distance
 * is exactly 136 bytes. buf[128] is big enough to hold the "%N$p" sweep
 * plus a marker prefix, letting the solve locate the canary robustly.
 *
 * LAB USE ONLY. Compiled with -fstack-protector-strong -no-pie -O1.
 */
#include "harness.h"
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

/* Marker sink. In the field this is where a ret2libc chain (system + /bin/sh
 * string, resolved from the leaked libc pointer) actually lands; for the CI
 * test we use a fixed local win() so pop-shell success is a file check.
 * Uses raw syscall write() to avoid libc SSE alignment on entry. */
void win(void)
{
    static const char pwn[] = "PWNED-CANARY-LEAK\n";
    write(1, pwn, sizeof pwn - 1);
    int fd = open("pwn.marker", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd >= 0) { write(fd, pwn, sizeof pwn - 1); close(fd); }
    _exit(0);
}

/* Explicit 136-byte pad-to-canary layout so the solve has a single stable
 * number. Modelled after real code that keeps a small handle/fd alongside
 * the input buffer. */
struct vuln_frame {
    char buf[128];
    long pad;
};

__attribute__((noinline))
void vuln(void)
{
    struct vuln_frame f;
    f.pad = 0;

    /* Step 1: read a format string. fgets stops at newline or 127 bytes. */
    if (!fgets(f.buf, 128, stdin))
        return;

    /* Step 2: the bug -- user-controlled format. printf reads varargs from
     * rsi/rdx/rcx/r8/r9 and then from our stack frame, so %N$p walks the
     * frame that contains the canary. */
    printf(f.buf);
    fflush(stdout);
    write(1, "---LEAK-DONE---\n", 16);

    /* Step 3: linear stack overflow into the same buffer. 512 >> 128 is the
     * vulnerability; the canary check on `ret` is the mitigation. */
    ssize_t n = read(0, f.buf, 512);
    (void)n;

    /* keep pad live so the compiler cannot fold it away */
    if (f.pad == 0xdeadbeefcafebabeL) puts("no");
}

int main(void)
{
    setup_io();
    vuln();
    return 0;
}
