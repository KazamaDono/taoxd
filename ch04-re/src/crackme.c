/* ch04-re/src/crackme.c — lab-only teaching target. See ../ETHICS.md. */
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#define KEYLEN 16

/* Expected transformed bytes: a rolling XOR of the real key. */
static const uint8_t target[KEYLEN] = {
    0x22, 0x4a, 0x60, 0x0e, 0x22, 0x0e, 0x5f, 0x50,
    0x55, 0x69, 0x3c, 0x2a, 0x13, 0x12, 0x0f, 0x08,
};

static int check_serial(const char *s)
{
    if (strlen(s) != KEYLEN)                              /* ❶ */
        return 0;

    uint8_t prev = 0x2a;                                  /* ❷ IV */
    for (int i = 0; i < KEYLEN; i++) {
        uint8_t c = (uint8_t)s[i] ^ (uint8_t)(0x5a + i) ^ prev;  /* ❸ */
        if (c != target[i])                               /* ❹ */
            return 0;
        prev = c;                                         /* ❺ feedback */
    }
    return 1;
}

int main(int argc, char **argv)
{
    puts("=== ch04 crackme: recover the key ===");
    if (argc != 2) {
        fprintf(stderr, "usage: %s <key>\n", argv[0]);
        return 2;
    }
    puts(check_serial(argv[1]) ? "Correct! The key is valid."
                               : "Nope. Try again.");
    return check_serial(argv[1]) ? 0 : 1;
}
