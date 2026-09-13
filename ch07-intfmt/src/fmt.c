/* ch07-intfmt/src/fmt.c  -- deliberately vulnerable format-string lab.
 * LAB ONLY. Built -no-pie -fno-stack-protector and WITHOUT _FORTIFY_SOURCE
 * so the classic printf(user) bug is reachable and %n works (see Makefile). */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int authorized = 0;                       /* ❶ fixed-address write target */

static void vuln(void)
{
    char buf[128];
    ssize_t n = read(0, buf, sizeof buf - 1);
    if (n <= 0) return;
    buf[n] = '\0';
    if (n && buf[n - 1] == '\n') buf[n - 1] = '\0';

    printf(buf);                          /* ❷ THE BUG: user controls the format */

    if (authorized) {                     /* ❸ win condition flipped by %n */
        puts("authorized: spawning shell");
        system("/bin/sh");
    } else {
        printf("denied (authorized=%d)\n", authorized);
    }
}

int main(void)
{
    setvbuf(stdout, NULL, _IONBF, 0);
    puts("ch07 fmt lab: speak, and I will print you.");
    vuln();
    return 0;
}
