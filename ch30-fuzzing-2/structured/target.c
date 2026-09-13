/* ch30-fuzzing-2/structured/target.c — toy `parse_record` with a planted
 * bug the FDP harness in lst-fdp reaches within minutes.
 *
 * Layout expected:
 *   version : uint8_t in [1,4]
 *   name    : arbitrary bytes, length <= 64
 *   payload : arbitrary bytes
 *
 * Bug (version == 3): the first four payload bytes are read as a big-endian
 * "declared length" and memcpy'd into a fixed 32-byte stack buffer with no
 * bound check against sizeof(stackbuf). Any payload of five or more bytes
 * whose declared length exceeds 32 overflows the buffer; ASan catches it. */

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int parse_record(uint8_t version,
                 const char *name, size_t name_len,
                 const uint8_t *payload, size_t payload_len) {
    if (version < 1 || version > 4) return -1;
    if (name_len > 64) return -1;

    volatile char stackbuf[32];

    switch (version) {
        case 1:
        case 2: {
            unsigned s = 0;
            for (size_t i = 0; i < payload_len; i++) s += payload[i];
            return (int)(s & 0xff);
        }
        case 3: {
            if (payload_len < 4) return -1;
            uint32_t declared = ((uint32_t)payload[0] << 24) |
                                ((uint32_t)payload[1] << 16) |
                                ((uint32_t)payload[2] <<  8) |
                                ((uint32_t)payload[3]);
            size_t avail = payload_len - 4;
            size_t n = declared < avail ? declared : avail;
            /* BUG: no check that n <= sizeof(stackbuf). */
            memcpy((void *)stackbuf, payload + 4, n);
            return (int)stackbuf[0];
        }
        case 4:
            return name_len ? (int)(unsigned char)name[0] : 0;
    }
    return 0;
}

#ifdef PARSE_RECORD_MAIN
/* Deterministic crash reproducer used by `make test`. Feeds a version-3
 * record whose declared length is 200 into a 32-byte buffer. ASan reports
 * a stack-buffer-overflow and exits non-zero; the Makefile inverts that
 * into a passing test. */
int main(void) {
    uint8_t payload[64];
    memset(payload, 0x41, sizeof(payload));
    payload[0] = 0; payload[1] = 0; payload[2] = 0; payload[3] = 200;
    (void)parse_record(3, "n", 1, payload, sizeof(payload));
    puts("MED_CH30_BUG_NOT_TRIGGERED");
    return 0;
}
#endif
