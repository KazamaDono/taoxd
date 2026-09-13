/* ch11-canary/relro/relro_demo.c
 *
 * One source, built two ways (relro_partial and relro_full), to show what
 * RELRO actually changes. The program hands us a textbook arbitrary write
 * primitive: read a target address and a value from stdin, poke the value,
 * then call puts("/bin/sh"). Under PARTIAL RELRO, .got.plt is writable, so
 * pointing puts@got.plt at win() turns the follow-up puts() call into a
 * win() call. Under FULL RELRO (-z,now), .got.plt is mprotect()ed R/O
 * before main runs, so the same write segfaults.
 *
 * The two binaries are identical source -- Chapter 11's whole point is that
 * ONE linker flag (-z,now) decides which behaviour you get.
 *
 * LAB USE ONLY.
 */
#include "harness.h"
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

/* Marker sink -- if the exploit lands, control reaches here via puts@got.plt
 * being pointed at &win. Raw syscalls to sidestep libc alignment on entry. */
void win(void)
{
    static const char pwn[] = "PWNED-RELRO-PARTIAL\n";
    int fd = open("pwn.marker", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd >= 0) { write(fd, pwn, sizeof pwn - 1); close(fd); }
    write(1, pwn, sizeof pwn - 1);
    _exit(0);
}

int main(void)
{
    setup_io();
    puts("ready");                          /* forces puts@got.plt to resolve
                                             * under lazy binding (partial),
                                             * or was already resolved (full) */

    unsigned long addr = 0, val = 0;
    if (scanf("%lx %lx", &addr, &val) != 2) {
        fprintf(stderr, "bad input\n");
        return 1;
    }

    /* The arbitrary-write primitive. Under partial RELRO, .got.plt is R/W
     * and this succeeds; under full RELRO, .got.plt is R/O and this SIGSEGVs
     * on the write instruction itself. */
    *(volatile unsigned long *)(uintptr_t)addr = val;

    /* If the write succeeded and we overwrote puts@got.plt with &win, this
     * call jumps to win() instead of puts(). puts() ignores its argument for
     * our purposes (win() takes no args and never returns). */
    puts("/bin/sh");
    return 0;
}
