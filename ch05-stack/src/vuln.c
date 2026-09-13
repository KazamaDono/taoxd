/* ch05-stack/src/vuln.c  — lab-only teaching target. See ../ETHICS.md.
 * Built two ways by ../Makefile: mitigations OFF (exploitable) and the
 * modern default ON (hardened). The overflow is identical; only the
 * defenses differ. */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/* The function the program never calls on any legitimate path. ret2win
 * redirects execution here; system("/bin/sh") is the oracle make test drives. */
void win(void)                                   /* ❶ */
{
    puts("[win] control-flow hijacked -- popping a shell");
    fflush(stdout);
    system("/bin/sh");
}

/* Reading through a noinline helper hides name[]'s compile-time size from
 * the call site, so -D_FORTIFY_SOURCE cannot swap in a bounds-checked
 * __read_chk(). The overflow therefore still happens on the hardened build
 * (where the canary catches it), not just the unprotected one. */
__attribute__((noinline))
static ssize_t fill(int fd, void *p, size_t n)   /* ❷ */
{
    return read(fd, p, n);
}

void greet(void)
{
    char name[64];                               /* ❸ the overflowable buffer */
    fputs("name> ", stdout);
    fill(0, name, 512);                          /* ❹ 512 bytes into a 64-byte buffer */
    printf("hello, %s\n", name);
}

int main(void)
{
    setvbuf(stdout, NULL, _IONBF, 0);
    greet();
    return 0;
}
