/* proto.h — shared protocol declarations for the ch31 lab. */
#ifndef PROTO_H
#define PROTO_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint32_t magic;
    uint32_t len;
    char     data[512];
} hdr_t;

typedef struct session {
    int id;
    int fd;
} session_t;

int  net_read_header(int fd, hdr_t *out);
void log_id(int id);
void parse_record(const hdr_t *hdr);
void handle_request(int fd);
void session_close(session_t *sess);

#endif
