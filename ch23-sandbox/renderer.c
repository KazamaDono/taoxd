/* ch23-sandbox/renderer.c — sets up the socketpair, forks the broker,
 * then execs the exploit payload for the demo.
 *
 * In real Chromium the renderer would be a JS engine that already fell
 * to an earlier chain and already had the zygote's seccomp filter
 * installed. Here the "renderer" is just the launcher: the compiled
 * exploit binary installs the identical filter as its first act (see
 * exploit.c), so the syscalls it makes after that point are exactly
 * the ones a jailed renderer would be allowed to make. That keeps
 * this file free of exec/seccomp ordering trickery and makes CI
 * deterministic. */
#define _GNU_SOURCE
#include "ipc.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

extern int broker_main(int cli);

#define BROKER_FD 3

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "usage: %s <exploit-binary>\n", argv[0]);
        return 2;
    }
    int sv[2];
    if (socketpair(AF_UNIX, SOCK_SEQPACKET, 0, sv) < 0) {
        perror("socketpair"); return 2;
    }

    pid_t pid = fork();
    if (pid < 0) { perror("fork"); return 2; }
    if (pid == 0) {
        /* Broker child: keeps sv[1], closes the renderer end. */
        close(sv[0]);
        int rc = broker_main(sv[1]);
        _exit(rc == 0 ? 0 : 1);
    }

    /* Renderer parent: close broker end, place socket at BROKER_FD. */
    close(sv[1]);
    if (sv[0] != BROKER_FD) {
        if (dup2(sv[0], BROKER_FD) < 0) { perror("dup2"); return 2; }
        close(sv[0]);
    }

    /* Exec the payload; it will install the seccomp jail on itself
     * (matching what the zygote would have done in production) and
     * then attempt the escape via the broker socket at fd 3. */
    execl(argv[1], argv[1], (char *)NULL);
    perror("execl exploit");
    _exit(127);
}
