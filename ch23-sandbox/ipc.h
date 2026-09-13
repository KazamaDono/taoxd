/* ch23-sandbox/ipc.h — shared message layout for broker/renderer/exploit. */
#ifndef CH23_IPC_H
#define CH23_IPC_H

#include <stddef.h>
#include <stdint.h>

enum { OP_OPEN_ASSET = 1, OP_RUN_HELPER = 2, OP_STATUS = 3 };

struct msg {
    uint32_t op;
    uint32_t len;                /* payload length, bounded            */
    char     data[512];          /* path for OPEN_ASSET, name for RUN */
};

int  send_msg(int fd, const struct msg *m);
int  recv_msg(int fd, struct msg *m);
int  send_fd(int fd, int payload_fd);
int  recv_fd(int fd);
int  send_status(int fd, int status);

#endif
