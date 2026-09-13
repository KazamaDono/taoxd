/* ch02-lab/hello_overflow.c
 * The lab's "hello world": the smallest overflow that proves your toolchain,
 * debugger, and pwntools can build, run, and DRIVE a binary end to end.
 * INSECURE ON PURPOSE — lab only. The real stack chapter is Chapter 5. */
#include <stdio.h>
#include <unistd.h>

/* win() is never called by the program's own control flow. If you see its
 * token, you redirected execution here. We use raw write()/_exit() (thin
 * syscall wrappers) so it runs regardless of stack alignment. */
void win(void)
{
    write(1, "CH02_LAB_OK\n", 12);
    _exit(0);
}

void vuln(void)
{
    char name[64];                                 /* 64-byte buffer, no canary */
    printf("what's your name? ");
    fflush(stdout);
    read(0, name, 256);                            /* reads up to 256 into 64 */
    printf("hello, %.64s\n", name);
    fflush(stdout);
}

int main(void)
{
    setvbuf(stdout, NULL, _IONBF, 0);              /* unbuffered for pwntools */
    vuln();
    return 0;
}
