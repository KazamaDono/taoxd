/* ch30-fuzzing-2/grammar/target/config_parser.c
 *
 * Toy parser for the mini-language produced by ../generate.py. Reads one
 * program from argv[1] (or stdin) and walks it.
 *
 * Planted bug: `parse_group_name` copies the identifier that follows the
 * "group" keyword into an 8-byte fixed buffer with no length check. Any
 * program of the form
 *      group <name-longer-than-7-chars> { ... }
 * overflows the buffer; ASan catches it. The generator in generate.py
 * ships identifiers of length <= 5 by design, so this bug is only reached
 * either (a) by editing IDS to include a longer identifier, per Exercise 3,
 * or (b) by feeding a crafted input — which the Makefile's `test` target
 * does deterministically. */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int parse_group_name(const char *p) {
    /* Skip "group" then whitespace. */
    p += 5;
    while (*p == ' ' || *p == '\t') p++;

    const char *start = p;
    while (*p && !isspace((unsigned char)*p) && *p != '{') p++;
    size_t n = (size_t)(p - start);

    char name[8];                       /* deliberately tiny */
    memcpy(name, start, n);             /* BUG: no bound check against 8 */
    name[n < sizeof(name) ? n : sizeof(name) - 1] = 0;
    return (int)name[0];
}

static void walk(const char *src) {
    const char *p = src;
    while ((p = strstr(p, "group ")) != NULL) {
        (void)parse_group_name(p);
        p += 6;
    }
}

int main(int argc, char **argv) {
    FILE *f = (argc > 1) ? fopen(argv[1], "rb") : stdin;
    if (!f) { perror("open"); return 1; }
    static char buf[1 << 16];
    size_t n = fread(buf, 1, sizeof(buf) - 1, f);
    buf[n] = 0;
    if (f != stdin) fclose(f);
    walk(buf);
    puts("MED_CH30_BUG_NOT_TRIGGERED");
    return 0;
}
