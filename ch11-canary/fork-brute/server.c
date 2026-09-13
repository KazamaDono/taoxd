/* ch11-canary/fork-brute/server.c
 *
 * Deliberately vulnerable fork-model TCP server on 127.0.0.1:4011. The parent
 * accept()s each connection and fork()s a child to handle it -- so every
 * child inherits the parent's TCB, which holds the per-thread stack_guard.
 * That is the crack the byte-by-byte canary brute widens.
 *
 * The bug: handle() recv()s 200 bytes into a 64-byte buffer, then writes
 * "ok\n" and returns. The canary check on return is what the brute probes:
 * a child that acks "ok" got the canary right; a child that dies without
 * acking got it wrong.
 *
 * The stack layout is nailed to 72 bytes buf-to-canary via a struct pad --
 * matching Chapter 11's OFFSET = BUF_LEN + 8 exactly.
 *
 * LAB USE ONLY. Compiled -fstack-protector-strong so every child has the
 * canary in the way; -no-pie so win()'s address is a fixed constant.
 */
#include "harness.h"
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <signal.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

#define PORT 4011

/* Marker sink -- the pwn.marker file is the CI oracle. Runs in the child
 * after the return address is hijacked. Raw syscalls to avoid libc SSE
 * alignment fuss on the freshly-ret-to entry. */
void win(void)
{
    static const char pwn[] = "PWNED-FORK-BRUTE\n";
    int fd = open("pwn.marker", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd >= 0) { write(fd, pwn, sizeof pwn - 1); close(fd); }
    write(2, pwn, sizeof pwn - 1);
    _exit(0);
}

/* Explicit 72-byte pad-to-canary layout: char buf[64] + long pad = 72. */
struct handle_frame {
    char buf[64];
    long pad;
};

__attribute__((noinline))
static void handle(int cfd)
{
    struct handle_frame f;
    f.pad = (long)cfd;                    /* keep pad live */

    /* THE BUG: 200 bytes read into a 64-byte buffer. Crosses pad, the
     * canary at f+72, saved rbp at f+80, and the return address at f+88. */
    ssize_t n = recv(cfd, f.buf, 200, 0);
    (void)n;

    write(cfd, "ok\n", 3);                /* the brute's oracle */
    if (f.pad == 0xdeadbeefcafebabeL)     /* prevent DCE */
        write(cfd, "?", 1);
}

int main(void)
{
    signal(SIGCHLD, SIG_IGN);             /* auto-reap children, no zombies */
    signal(SIGPIPE, SIG_IGN);

    int sfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sfd < 0) { perror("socket"); return 1; }

    int one = 1;
    setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof one);

    struct sockaddr_in a = {
        .sin_family      = AF_INET,
        .sin_port        = htons(PORT),
        .sin_addr.s_addr = htonl(INADDR_LOOPBACK),
    };
    if (bind(sfd, (struct sockaddr *)&a, sizeof a) < 0) { perror("bind"); return 1; }
    if (listen(sfd, 32) < 0) { perror("listen"); return 1; }

    fprintf(stderr, "fork-brute server ready on 127.0.0.1:%d (pid %d)\n",
            PORT, (int)getpid());
    fflush(stderr);

    for (;;) {
        int cfd = accept(sfd, NULL, NULL);
        if (cfd < 0) { if (errno == EINTR) continue; perror("accept"); break; }
        pid_t p = fork();
        if (p == 0) {
            close(sfd);
            handle(cfd);
            close(cfd);
            _exit(0);
        } else if (p < 0) {
            perror("fork");
            close(cfd);
        } else {
            close(cfd);
        }
    }
    return 0;
}
