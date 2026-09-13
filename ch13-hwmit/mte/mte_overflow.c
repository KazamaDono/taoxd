/* Build: clang-18 --target=aarch64-linux-gnu -O1 -march=armv8.5-a+memtag \
 *          -fsanitize=memtag-stack -fsanitize=memtag-heap -fuse-ld=lld \
 *          mte_overflow.c -o mte_overflow
 * Run:   qemu-aarch64 -cpu max ./mte_overflow */
#define _GNU_SOURCE
#include <sys/mman.h>
#include <sys/prctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#ifndef PR_MTE_TCF_SYNC
#define PR_MTE_TCF_SYNC (1UL << 1)                            /* ❶ */
#define PR_SET_TAGGED_ADDR_CTRL 55
#endif
#ifndef SEGV_MTESERR
#define SEGV_MTESERR 9
#endif

static void on_sigsegv(int s, siginfo_t *si, void *uc) {
    dprintf(2, "SIGSEGV @ %p  si_code=%d  (%s)\n",
        si->si_addr, si->si_code,
        si->si_code == SEGV_MTESERR ? "MTE tag mismatch (sync)" : "other");
    _exit(1);
}

int main(void) {
    struct sigaction sa = { .sa_flags = SA_SIGINFO, .sa_sigaction = on_sigsegv };
    sigemptyset(&sa.sa_mask); sigaction(SIGSEGV, &sa, NULL);

    if (prctl(PR_SET_TAGGED_ADDR_CTRL,                        /* ❷ */
              PR_MTE_TCF_SYNC | (0xfffe << 3), 0, 0, 0) < 0) {
        perror("prctl(MTE)"); return 2;
    }

    /* Two adjacent 16-byte chunks; glibc-MTE (or scudo) gives them different tags. */
    char *a = malloc(16);
    char *b = malloc(16);                                     /* ❸ */
    printf("a=%p b=%p (top-byte tags differ)\n", a, b);

    memset(a, 'A', 16);            /* in-bounds: tag matches, ok */
    a[16] = 'X';                   /* ❹ one byte into b's granule: MISMATCH */
    puts("unreachable");
    return 0;
}
