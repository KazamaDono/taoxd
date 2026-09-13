/* ch29-fuzzing-1/src/csv_parse.h — lab-only parser with planted bugs. */
#ifndef CSV_PARSE_H
#define CSV_PARSE_H

#include <stddef.h>

typedef struct csv_row {
    char   **fields;
    size_t   n_fields;
} csv_row;

/* Parse one CSV record from `data` (length `len`). Returns 0 on success and
 * fills *out; returns -1 on malformed input. Caller must csv_row_free(out). */
int  csv_parse_row(const unsigned char *data, size_t len, csv_row *out);
void csv_row_free(csv_row *row);

#endif
