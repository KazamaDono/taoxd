/* stats.c — false-positive variant used to demonstrate the
 * bounded-by-helper triage pattern discussed at the end of the chapter.
 *
 * The taint from read() reaches memcpy(), but check_len() bounds it
 * against sizeof(cache_key) first.  The coarse barrier predicate in the
 * shipped CodeQL query does not model the helper's return value, so it
 * still flags this site — the refinement exercise (Exercise 3) teaches
 * the reader to fix that. */
#include <string.h>
#include <unistd.h>
#include <stddef.h>

static int check_len(size_t l, size_t cap)
{
    return l <= cap;
}

void stats_update(int fd)
{
    char buf[256];
    ssize_t n = read(fd, buf, sizeof(buf));        /* taint source */
    if (n < 4) return;

    unsigned int rlen;
    memcpy(&rlen, buf, sizeof(rlen));

    char cache_key[128];
    if (!check_len(rlen, sizeof(cache_key)))
        return;                                    /* bound applied — but via helper */

    memcpy(cache_key, buf + 4, rlen);              /* FALSE-POSITIVE SINK */
}
