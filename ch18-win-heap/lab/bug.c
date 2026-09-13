/* excerpt — full source in ch18-win-heap/lab/bug.c.
 * Build: cl /Zi /Od /GS- /MT bug.c /link /SUBSYSTEM:CONSOLE
 * The /GS- disables the compiler stack canary so the *stack* is not the
 * story; the vulnerability is in the *heap*, and we want the reader's
 * eye on that.                                                        */
#include <windows.h>
#include <string.h>

typedef struct { char name[0x40]; } small_t;                          /* ❶ */

/* Copies `src` into a freshly allocated 0x40-byte block, but treats an
 * attacker-supplied length as trusted — a size-calculation bug of a class
 * we surveyed in {{ref:ch:ch-intfmt}}. `n` may exceed 0x40.           ❷ */
void vuln_copy(const char *src, SIZE_T n, small_t **out)
{
    small_t *p = (small_t *)HeapAlloc(GetProcessHeap(), 0, sizeof(*p));
    memcpy(p->name, src, n);                                          /* ❸ */
    *out = p;
}
