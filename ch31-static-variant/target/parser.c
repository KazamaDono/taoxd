/* parser.c — three memcpy call sites, two safe and one not.
 *
 * The Semgrep rule in semgrep-rules/unchecked-memcpy.yaml (Listing 31-1)
 * flags exactly the one in parse_record() and correctly excludes the two
 * that are preceded by a sizeof(dest) bound. */
#include <string.h>
#include <stddef.h>

#include "proto.h"

void parse_record(const hdr_t *hdr)
{
    /* --- Safe call #1: length bounded by sizeof(safe1) before use. --- */
    char   safe1[128];
    size_t n1 = hdr->len;
    if (n1 <= sizeof(safe1)) {
        /* bound checked — Semgrep pattern-not #2 fires and excludes us. */
    } else {
        return;
    }
    memcpy(safe1, hdr->data, n1);

    /* --- Safe call #2: strict-less-than variant. --- */
    char   safe2[128];
    size_t n2 = hdr->len;
    if (n2 < sizeof(safe2)) {
        /* bound checked — Semgrep pattern-not #3 fires and excludes us. */
    } else {
        return;
    }
    memcpy(safe2, hdr->data, n2);

    /* --- Unsafe call: no bound.  This is the seeded bug. --- */
    char buf[256];
    memcpy(buf, hdr->data, hdr->len);            /* SEMGREP HIT */
}
