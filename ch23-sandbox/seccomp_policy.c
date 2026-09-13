/* ch23-sandbox/seccomp_policy.c — lab-only. Full source in ch23-sandbox/. */
#include <linux/audit.h>
#include <linux/filter.h>
#include <linux/seccomp.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <errno.h>
#include <stddef.h>
#include <unistd.h>

/* Emit a BPF check for a specific syscall number.                    */
#define ALLOW_SYSCALL(nr)                                             \
    BPF_JUMP(BPF_JMP+BPF_JEQ+BPF_K, (nr), 0, 1),                      \
    BPF_STMT(BPF_RET+BPF_K, SECCOMP_RET_ALLOW)

int install_renderer_filter(void)
{
    struct sock_filter filter[] = {
        /* Load arch, refuse anything but x86_64 to defeat            */
        /* 32-bit compat confusion.                                    */
        BPF_STMT(BPF_LD+BPF_W+BPF_ABS,                                /* [1] */
                 offsetof(struct seccomp_data, arch)),
        BPF_JUMP(BPF_JMP+BPF_JEQ+BPF_K, AUDIT_ARCH_X86_64, 1, 0),
        BPF_STMT(BPF_RET+BPF_K, SECCOMP_RET_KILL_PROCESS),

        /* Load nr into the accumulator for the comparisons below.    */
        BPF_STMT(BPF_LD+BPF_W+BPF_ABS,
                 offsetof(struct seccomp_data, nr)),

        ALLOW_SYSCALL(SYS_read),                                      /* [2] */
        ALLOW_SYSCALL(SYS_write),
        ALLOW_SYSCALL(SYS_recvmsg),
        ALLOW_SYSCALL(SYS_sendmsg),
        ALLOW_SYSCALL(SYS_close),
        ALLOW_SYSCALL(SYS_fchmod),
        ALLOW_SYSCALL(SYS_mmap),
        ALLOW_SYSCALL(SYS_mprotect),
        ALLOW_SYSCALL(SYS_munmap),
        ALLOW_SYSCALL(SYS_brk),
        ALLOW_SYSCALL(SYS_futex),
        ALLOW_SYSCALL(SYS_rt_sigreturn),
        ALLOW_SYSCALL(SYS_exit),
        ALLOW_SYSCALL(SYS_exit_group),

        /* Default: fail closed. KILL_PROCESS makes bypass attempts   */
        /* extremely noisy in dmesg during audits.                    */
        BPF_STMT(BPF_RET+BPF_K, SECCOMP_RET_KILL_PROCESS),            /* [3] */
    };
    struct sock_fprog prog = {
        .len = sizeof(filter) / sizeof(filter[0]),
        .filter = filter,
    };
    /* NO_NEW_PRIVS is a prerequisite for an unprivileged seccomp     */
    /* install; without it setuid binaries reached by execve would    */
    /* let a filtered process escalate.                                */
    if (prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0) < 0) return -1;        /* [4] */
    if (prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &prog) < 0)        /* [5] */
        return -1;
    return 0;
}
