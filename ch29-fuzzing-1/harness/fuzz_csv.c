/* ch29-fuzzing-1/harness/fuzz_csv.c
 * Build (libFuzzer): clang-18 -g -O1 -fsanitize=fuzzer,address,undefined \
 *   -I../src ../src/csv_parse.c fuzz_csv.c -o fuzz_csv_libfuzzer
 * Build (AFL++):     AFL_USE_ASAN=1 afl-clang-fast -g -O1 \
 *   -I../src ../src/csv_parse.c fuzz_csv.c \
 *   $(afl-config --libdir)/libAFLDriver.a -o fuzz_csv_afl
 */
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "csv_parse.h"

/* Cheap cap: reject inputs the target was never designed to see, so the       */
/* fuzzer does not waste energy on 4 MiB of random bytes.                      */
enum { CSV_MAX_INPUT = 64 * 1024 };                                     /* ❶ */

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)            /* ❷ */
{
    if (size == 0 || size > CSV_MAX_INPUT)
        return 0;                                                       /* ❸ */

    csv_row row = {0};
    if (csv_parse_row(data, size, &row) == 0)
        csv_row_free(&row);                                             /* ❹ */

    return 0;                                                           /* ❺ */
}
