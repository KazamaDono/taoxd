/* ch23-sandbox/ipc.c — sendmsg/recvmsg helpers with SCM_RIGHTS. */
#define _GNU_SOURCE
#include "ipc.h"

#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

int send_msg(int fd, const struct msg *m)
{
    struct iovec iov = { .iov_base = (void *)m, .iov_len = sizeof(*m) };
    struct msghdr h  = { .msg_iov = &iov, .msg_iovlen = 1 };
    ssize_t n;
    do { n = sendmsg(fd, &h, 0); } while (n < 0 && errno == EINTR);
    return (n == (ssize_t)sizeof(*m)) ? 0 : -1;
}

int recv_msg(int fd, struct msg *m)
{
    struct iovec iov = { .iov_base = m, .iov_len = sizeof(*m) };
    struct msghdr h  = { .msg_iov = &iov, .msg_iovlen = 1 };
    ssize_t n;
    do { n = recvmsg(fd, &h, 0); } while (n < 0 && errno == EINTR);
    return (n == (ssize_t)sizeof(*m)) ? 0 : -1;
}

int send_fd(int fd, int payload_fd)
{
    struct msg m = { .op = OP_STATUS, .len = 0 };
    struct iovec iov = { .iov_base = &m, .iov_len = sizeof(m) };

    union {
        struct cmsghdr cmsg;
        char buf[CMSG_SPACE(sizeof(int))];
    } u;
    memset(&u, 0, sizeof(u));

    struct msghdr h = {
        .msg_iov = &iov, .msg_iovlen = 1,
        .msg_control = u.buf, .msg_controllen = sizeof(u.buf),
    };
    struct cmsghdr *c = CMSG_FIRSTHDR(&h);
    c->cmsg_level = SOL_SOCKET;
    c->cmsg_type  = SCM_RIGHTS;
    c->cmsg_len   = CMSG_LEN(sizeof(int));
    memcpy(CMSG_DATA(c), &payload_fd, sizeof(int));

    ssize_t n;
    do { n = sendmsg(fd, &h, 0); } while (n < 0 && errno == EINTR);
    return (n == (ssize_t)sizeof(m)) ? 0 : -1;
}

int recv_fd(int fd)
{
    struct msg m;
    struct iovec iov = { .iov_base = &m, .iov_len = sizeof(m) };
    union {
        struct cmsghdr cmsg;
        char buf[CMSG_SPACE(sizeof(int))];
    } u;
    memset(&u, 0, sizeof(u));

    struct msghdr h = {
        .msg_iov = &iov, .msg_iovlen = 1,
        .msg_control = u.buf, .msg_controllen = sizeof(u.buf),
    };
    ssize_t n;
    do { n = recvmsg(fd, &h, 0); } while (n < 0 && errno == EINTR);
    if (n != (ssize_t)sizeof(m)) return -1;

    for (struct cmsghdr *c = CMSG_FIRSTHDR(&h); c; c = CMSG_NXTHDR(&h, c)) {
        if (c->cmsg_level == SOL_SOCKET && c->cmsg_type == SCM_RIGHTS) {
            int payload_fd;
            memcpy(&payload_fd, CMSG_DATA(c), sizeof(int));
            return payload_fd;
        }
    }
    return -1;
}

int send_status(int fd, int status)
{
    struct msg m = { .op = OP_STATUS, .len = sizeof(int) };
    memcpy(m.data, &status, sizeof(int));
    return send_msg(fd, &m);
}
