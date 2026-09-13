/* ch12-cfi/cfi-icall/reuse.c
 * Build: clang -O1 -flto -fvisibility=hidden -fsanitize=cfi \
 *              -fsanitize-trap=cfi -o reuse reuse.c */
#include <stdio.h>
#include <string.h>

typedef int (*handler_t)(int, const char *);

/* Both handlers share a signature. Both are address-taken.
 * That is enough for Clang cfi-icall to consider them equivalent. */
static int handle_msg(int fd, const char *msg) {                    /* ❶ */
    printf("[msg on fd=%d] %s\n", fd, msg);
    return 0;
}

static int admin_shell(int fd, const char *msg) {                   /* ❷ */
    /* Imagine: exec a shell, drop privileges, whatever the
     * attacker wanted the "admin" path to do. */
    printf("[!!! admin_shell reached, fd=%d, arg=%s]\n", fd, msg);
    return 0;
}

/* Address-taken table: both entries live in the CFI set for handler_t. */
static handler_t handlers[2] = { handle_msg, admin_shell };         /* ❸ */

int main(int argc, char **argv) {
    handler_t cb = handlers[0];                       /* the legit one */

    /* Simulate a heap-corruption primitive that swaps the callback
     * slot for the SAME-TYPED admin function. In a real target this
     * would come from a UAF or an OOB write into the struct that
     * holds `cb`. */
    memcpy(&cb, &handlers[1], sizeof cb);                           /* ❹ */

    cb(3, "hello");                                                 /* ❺ */
    return 0;
}
