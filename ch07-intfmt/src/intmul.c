/* ch07-intfmt/src/intmul.c -- integer overflow -> undersized malloc ->
 * heap overflow into an adjacent object.  LAB ONLY.
 * stdin: header {u32 count; u32 nbytes; u32 pad}, then nbytes of data. */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

struct rec  { char data[32]; };                 /* 32-byte records        */
struct auth { long is_admin; char user[24]; };  /* the adjacent victim    */

static uint32_t rd_u32(void)
{
    uint32_t v = 0;
    if (fread(&v, 1, sizeof v, stdin) != sizeof v) exit(1);
    return v;
}

int main(void)
{
    uint32_t count  = rd_u32();                            /* attacker */
    uint32_t nbytes = rd_u32();                            /* attacker */
    (void) rd_u32();                                       /* padding  */

    uint32_t size = count * (uint32_t) sizeof(struct rec); /* ❶ 32-bit multiply */
    struct rec  *recs = malloc(size);                      /* ❷ undersized on wrap */
    struct auth *a    = calloc(1, sizeof *a);              /* ❸ victim, is_admin=0 */
    if (!recs || !a) return 1;

    fread(recs, 1, nbytes, stdin);                         /* ❹ writes nbytes, not size */

    if (a->is_admin) puts("ACCESS GRANTED");               /* ❺ */
    else             puts("access denied");
    return 0;
}
