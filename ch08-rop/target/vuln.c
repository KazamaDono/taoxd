/* ch08-rop/target/vuln.c - deliberately vulnerable lab target.
 * Lab use only. See ../../ETHICS.md.
 * Built NX-on, canary-off, PIE-off as a teaching choice (see Makefile). */
#include <stdio.h>
#include <unistd.h>

static void vuln(void)
{
    char buf[64];                      /* ❶ the buffer we overflow   */
    read(0, buf, 512);                 /* ❷ reads far past 64 bytes  */
    puts("thanks for the input");      /* ❸ runs before the ret      */
}

int main(void)
{
    setvbuf(stdout, NULL, _IONBF, 0);  /* unbuffered, so leaks flush */
    setvbuf(stdin,  NULL, _IONBF, 0);
    puts("=== ch08 rop lab: DEP/NX is on; inject no code ===");
    vuln();
    return 0;
}
