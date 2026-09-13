// harness.c — libFuzzer wrapper around notesd's request parser.
// Build: clang-18 -O1 -fsanitize=fuzzer,address,undefined \
//        -I ../target -o build/fuzz harness.c ../target/parse.c
#include <stddef.h>
#include <stdint.h>
#include <string.h>

// The one function we want to hit repeatedly; extracted from notesd
// by the reverser and linked into the fuzz build.
extern int notesd_parse_request(const uint8_t *buf, size_t len);

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {   // ❶
    if (size < 4) return 0;                                      // ❷
    // The wire format is: u16 op, u16 len, then len bytes.
    // Reject sizes the real server would reject at the socket layer,
    // so the fuzzer's coverage is spent on the parser, not the framer.
    uint16_t op  = (uint16_t)(data[0] | (data[1] << 8));         // ❸
    uint16_t len = (uint16_t)(data[2] | (data[3] << 8));
    if ((size_t)len + 4 > size || len > 4096) return 0;
    (void)op;
    return notesd_parse_request(data + 4, len);                  // ❹
}
