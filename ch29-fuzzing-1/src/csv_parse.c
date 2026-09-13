/* ch29-fuzzing-1/src/csv_parse.c — deliberately-vulnerable CSV parser.
 *
 * LAB USE ONLY. This translation unit intentionally contains three bugs so
 * that a coverage-guided fuzzer can rediscover them under AddressSanitizer:
 *
 *   BUG 1 (heap overflow via integer wrap): the per-field byte buffer is
 *          capped by a `unsigned short`. Once the buffer grows past 32 KiB
 *          the doubling wraps to 0, we fall back to +1, and the next
 *          write is a heap-buffer-overflow WRITE past the small chunk.
 *
 *   BUG 2 (off-by-one WRITE in doubled-quote handling): when we see the
 *          escape "" inside a quoted field we blindly do
 *              buf[buf_len++] = '"';
 *          without checking against buf_cap first. If the quoted field
 *          exactly filled the buffer the store lands one byte past the
 *          heap chunk.
 *
 *   BUG 3 (heap-use-after-free via '\z' escape): the '\z' undocumented
 *          "reset" free()s the field buffer but forgets to null or
 *          reallocate it, so the next store on any input byte is a
 *          heap-use-after-free WRITE.
 *
 * None of these bugs should exist in production code. They are the point
 * of the chapter's fuzzing exercise.
 */
#include "csv_parse.h"

#include <stdlib.h>
#include <string.h>

int csv_parse_row(const unsigned char *data, size_t len, csv_row *out)
{
    if (!data || !out) return -1;
    out->fields = NULL;
    out->n_fields = 0;

    size_t   cap_fields = 8;
    char   **fields = (char **)calloc(cap_fields, sizeof(char *));
    if (!fields) return -1;
    size_t   n = 0;

    /* BUG 1: 16-bit capacity — will wrap past 32768. */
    unsigned short buf_cap = 16;
    char          *buf     = (char *)malloc(buf_cap);
    if (!buf) { free(fields); return -1; }
    size_t         buf_len = 0;

    int    in_quote = 0;
    size_t i        = 0;

    /* Optional UTF-8 BOM. */
    if (len >= 3 && data[0] == 0xEF && data[1] == 0xBB && data[2] == 0xBF)
        i = 3;

    for (; i < len; i++) {
        unsigned char c = data[i];
        unsigned char to_store = 0;
        int           store    = 0;

        if (!in_quote && c == '\\' && i + 1 < len) {
            unsigned char e = data[i + 1];
            if (e == 'n')      { to_store = '\n'; store = 1; i++; }
            else if (e == 't') { to_store = '\t'; store = 1; i++; }
            else if (e == 'z') {
                /* BUG 3: free without nulling/re-allocating. */
                free(buf);
                buf_len = 0;
                i++;
                continue;
            } else {
                to_store = c; store = 1;
            }
        } else if (c == '"') {
            if (in_quote && i + 1 < len && data[i + 1] == '"') {
                /* BUG 2: no bounds check before this store. */
                buf[buf_len++] = '"';
                i++;
                continue;
            }
            in_quote = !in_quote;
            continue;
        } else if (c == ',' && !in_quote) {
            /* End of field: NUL-terminate and dup. */
            if (buf_len >= buf_cap) {
                unsigned short nc = (unsigned short)(buf_cap + 1);
                char *tmp = (char *)realloc(buf, nc);
                if (!tmp) goto fail;
                buf = tmp; buf_cap = nc;
            }
            buf[buf_len] = '\0';
            if (n >= cap_fields) {
                cap_fields *= 2;
                char **tmp = (char **)realloc(fields,
                                              cap_fields * sizeof(char *));
                if (!tmp) goto fail;
                fields = tmp;
            }
            fields[n++] = strdup(buf);
            buf_len = 0;
            continue;
        } else {
            to_store = c; store = 1;
        }

        if (store) {
            if (buf_len >= buf_cap) {
                /* BUG 1: doubling wraps a unsigned short past 32 KiB. */
                unsigned short nc = (unsigned short)(buf_cap * 2);
                if (nc <= buf_cap) nc = (unsigned short)(buf_cap + 1);
                char *tmp = (char *)realloc(buf, nc);
                if (!tmp) goto fail;
                buf = tmp; buf_cap = nc;
            }
            buf[buf_len++] = to_store;
        }
    }

    /* Final field. */
    if (buf_len >= buf_cap) {
        unsigned short nc = (unsigned short)(buf_cap + 1);
        char *tmp = (char *)realloc(buf, nc);
        if (!tmp) goto fail;
        buf = tmp; buf_cap = nc;
    }
    buf[buf_len] = '\0';
    if (n >= cap_fields) {
        cap_fields = n + 1;
        char **tmp = (char **)realloc(fields, cap_fields * sizeof(char *));
        if (!tmp) goto fail;
        fields = tmp;
    }
    fields[n++] = strdup(buf);
    free(buf);

    out->fields  = fields;
    out->n_fields = n;
    return 0;

fail:
    for (size_t k = 0; k < n; k++) free(fields[k]);
    free(fields);
    free(buf);
    return -1;
}

void csv_row_free(csv_row *row)
{
    if (!row || !row->fields) return;
    for (size_t i = 0; i < row->n_fields; i++) free(row->fields[i]);
    free(row->fields);
    row->fields  = NULL;
    row->n_fields = 0;
}
