/* ch29-fuzzing-1/harness/replay_main.c
 *
 * Non-fuzzer entry point used by the coverage build (scripts/coverage.sh)
 * and by `make test`. It reads each argv file into a buffer and feeds it
 * to LLVMFuzzerTestOneInput exactly the way libFuzzer would. Deterministic,
 * side-effect free, no filesystem I/O beyond the argv files.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size);

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "usage: %s FILE [FILE...]\n", argv[0]);
        return 2;
    }
    for (int a = 1; a < argc; a++) {
        FILE *f = fopen(argv[a], "rb");
        if (!f) { perror(argv[a]); return 2; }
        if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return 2; }
        long n = ftell(f);
        if (n < 0) { fclose(f); return 2; }
        rewind(f);
        uint8_t *buf = (uint8_t *)malloc((size_t)n + 1);
        if (!buf) { fclose(f); return 2; }
        size_t got = fread(buf, 1, (size_t)n, f);
        fclose(f);
        LLVMFuzzerTestOneInput(buf, got);
        free(buf);
    }
    return 0;
}
