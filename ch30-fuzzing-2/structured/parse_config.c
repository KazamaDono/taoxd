/* ch30-fuzzing-2/structured/parse_config.c — toy config-text parser.
 *
 * Grammar (informal):
 *   config := stmt*
 *   stmt   := "version" "=" NUM ";"
 *           | KEY "=" VALUE ";"
 *           | "group" NAME "{" stmt* "}"
 *
 * Planted bug: `parse_group` recurses on every "{" with no depth limit and
 * a fat local frame, so a deeply-nested input reliably overflows the
 * pthread stack. ASan's stack-overflow detector catches it. This is the
 * archetypal bug an LPM harness discovers, because the mutator can freely
 * extend `repeated Group sub` inside a `Group`. */

#include <ctype.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *g_cur;
static const char *g_end;

static void skip_ws(void) {
    while (g_cur < g_end) {
        unsigned char c = (unsigned char)*g_cur;
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') { g_cur++; continue; }
        if (c == '#') { while (g_cur < g_end && *g_cur != '\n') g_cur++; continue; }
        break;
    }
}

static void consume_token(void) {
    while (g_cur < g_end) {
        char c = *g_cur;
        if (c == '{' || c == '}' || c == ';') return;
        g_cur++;
    }
}

static int parse_group(void) {
    /* deliberately fat frame so recursion depth burns stack fast */
    volatile char frame[512];
    frame[0] = 0;

    for (;;) {
        skip_ws();
        if (g_cur >= g_end) break;
        if (*g_cur == '}') { g_cur++; break; }
        if (*g_cur == '{') {
            g_cur++;
            (void)parse_group();  /* BUG: unbounded recursion */
            continue;
        }
        if (*g_cur == ';') { g_cur++; continue; }
        consume_token();
    }
    return (int)frame[0];
}

int parse_config(const char *text, size_t len) {
    if (!text || len == 0) return 0;
    g_cur = text;
    g_end = text + len;
    return parse_group();
}

#ifdef PARSE_CONFIG_MAIN
int main(void) {
    /* ~200000 open braces reliably overflows the default 8 MiB stack under
     * a 512-byte-frame recursion. ASan reports stack-overflow. */
    size_t N = 200000;
    char *buf = (char *)malloc(N + 1);
    if (!buf) return 2;
    memset(buf, '{', N);
    buf[N] = 0;
    (void)parse_config(buf, N);
    free(buf);
    puts("MED_CH30_BUG_NOT_TRIGGERED");
    return 0;
}
#endif
