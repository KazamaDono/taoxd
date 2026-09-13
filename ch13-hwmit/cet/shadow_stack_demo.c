/* Build: gcc -O2 -fno-omit-frame-pointer -fcf-protection=full shadow_stack_demo.c -o shstk_demo
 * Run:   GLIBC_TUNABLES=glibc.cpu.hwcaps=SHSTK ./shstk_demo
 * Needs: Intel Tiger Lake+ or AMD Zen 3+, Linux 6.6+, glibc 2.39. */
#define _GNU_SOURCE
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ucontext.h>

#ifndef SEGV_CPERR
#define SEGV_CPERR 10                             /* ❶ from linux/siginfo.h */
#endif

static void on_sigsegv(int sig, siginfo_t *si, void *uc) {
    const char *why =
        (si->si_code == SEGV_CPERR) ? "SHSTK/IBT (CET) violation"
      : (si->si_code == SEGV_MAPERR) ? "unmapped address"
                                     : "other";
    dprintf(2, "SIGSEGV @ %p  si_code=%d  (%s)\n",
            si->si_addr, si->si_code, why);
    _exit(1);
}

__attribute__((noinline))
static void victim(void) {
    volatile unsigned long *saved_ret;
    asm volatile ("lea 8(%%rbp), %0" : "=r"(saved_ret));  /* ❷ */
    *saved_ret = 0xdeadbeefUL;                            /* ❸ */
    /* ret executes here: CPU compares normal vs shadow, faults on CET */
}

int main(void) {
    struct sigaction sa = { .sa_flags = SA_SIGINFO, .sa_sigaction = on_sigsegv };
    sigemptyset(&sa.sa_mask);
    sigaction(SIGSEGV, &sa, NULL);
    puts("about to smash the saved return address...");
    victim();
    puts("unreachable on any modern kernel");
    return 0;
}
