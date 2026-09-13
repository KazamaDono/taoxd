/* handler.c — network-tainted memcpy sink (CodeQL) and a small UAF (weggli).
 *
 * Two independent seeded bugs live here:
 *
 *   1. handle_request() reads a header from the network in net.c, extracts
 *      hdr->len, and hands that length straight to memcpy().  The CodeQL
 *      taint query in Listing 31-3 walks the flow from recv() to memcpy().
 *
 *   2. session_close() frees a session pointer and then dereferences it.
 *      The weggli pattern in Listing 31-2 catches free(x); ... x->field;
 *      inside a single scope. */
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "proto.h"

static char scratch[4096];

void handle_request(int fd)
{
    hdr_t hdr;
    if (net_read_header(fd, &hdr) < 0)
        return;

    /* Tainted length flows in with no bound against sizeof(scratch). */
    size_t len = hdr.len;
    memcpy(scratch, hdr.data, len);              /* CODEQL SINK */
}

void session_close(session_t *sess)
{
    /* Freed here... */
    free(sess);
    /* ...and dereferenced two lines later.  weggli sees it. */
    log_id(sess->id);                             /* WEGGLI UAF HIT */
}
