/* net.c — network source of taint for the ch31 static-analysis lab. */
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>

#include "proto.h"

int net_read_header(int fd, hdr_t *out)
{
    /* Seeded taint source: the entire header (magic, len, data) is read
     * straight off the wire.  The CodeQL query in Listing 31-3 marks
     * argument 1 of recv() as tainted; that taint flows through hdr->len
     * into parser.c and eventually into memcpy() in handler.c. */
    ssize_t n = recv(fd, out, sizeof(*out), 0);        /* line 15 — taint source */
    if (n <= 0) return -1;
    return 0;
}

void log_id(int id)
{
    (void)id;   /* pretend to log; not important for the lab */
}
