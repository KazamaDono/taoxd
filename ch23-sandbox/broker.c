/* ch23-sandbox/broker.c — deliberately vulnerable lab broker. */
#define _GNU_SOURCE
#include "ipc.h"

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define ASSETS_DIR   "/tmp/lab-sandbox/assets/"
#define HELPERS_DIR  "/tmp/lab-sandbox/helpers/"

static void run_and_wait(int cli, const char *full)
{
    pid_t pid = fork();
    if (pid < 0) { send_status(cli, errno); return; }
    if (pid == 0) {
        execl(full, full, (char *)NULL);
        _exit(127);
    }
    int status = 0;
    waitpid(pid, &status, 0);
    send_status(cli, WIFEXITED(status) ? WEXITSTATUS(status) : -1);
}

static void handle_open_asset(int cli, const struct msg *m)
{
    char path[1024];
    if (m->len == 0 || m->len > sizeof(m->data)) {                    /* [1] */
        send_status(cli, EINVAL);
        return;
    }
    memcpy(path, m->data, m->len);
    path[m->len] = '\0';

    /* Reject empty and non-absolute inputs early.                    */
    if (path[0] != '/') {                                             /* [2] */
        send_status(cli, EINVAL);
        return;
    }

    /* THE BUG: prefix check without canonicalisation. Anything that  */
    /* textually begins with ASSETS_DIR is accepted, so                */
    /* "/tmp/lab-sandbox/assets/../helpers/pwn" passes.                */
    if (strncmp(path, ASSETS_DIR, strlen(ASSETS_DIR)) != 0) {         /* [3] */
        send_status(cli, EACCES);
        return;
    }

    int fd = open(path, O_RDWR | O_CREAT, 0644);                      /* [4] */
    if (fd < 0) { send_status(cli, errno); return; }

    send_fd(cli, fd);                                                 /* [5] */
    close(fd);
}

static void handle_run_helper(int cli, const struct msg *m)
{
    char name[64];
    if (m->len == 0 || m->len >= sizeof(name)) {
        send_status(cli, EINVAL); return;
    }
    memcpy(name, m->data, m->len); name[m->len] = '\0';

    for (size_t i = 0; i < m->len; i++) {                             /* [6] */
        char c = name[i];
        int ok = (c >= 'a' && c <= 'z') ||
                 (c >= '0' && c <= '9' && i > 0) ||
                 c == '_' || c == '-';
        if (!ok) { send_status(cli, EINVAL); return; }
    }

    char full[128];
    snprintf(full, sizeof(full), "%s%s", HELPERS_DIR, name);
    run_and_wait(cli, full);                                          /* [7] */
}

int broker_main(int cli)
{
    for (;;) {
        struct msg m;
        if (recv_msg(cli, &m) < 0) return 0;
        switch (m.op) {
        case OP_OPEN_ASSET: handle_open_asset(cli, &m); break;
        case OP_RUN_HELPER: handle_run_helper(cli, &m); break;
        default: send_status(cli, EINVAL); break;
        }
    }
}
