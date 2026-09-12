/* harness.h — tiny shared helpers for deliberately-vulnerable lab targets.
 * Lab use only. These targets are INSECURE ON PURPOSE. */
#ifndef MED_HARNESS_H
#define MED_HARNESS_H
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/* Make I/O unbuffered so exploit scripts see prompts immediately. */
static inline void setup_io(void) {
    setvbuf(stdin, NULL, _IONBF, 0);
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);
}

/* A conventional "you win" sink used by early chapters. Real exploitation
 * targets code reuse / shellcode instead; win() is a teaching scaffold. */
static inline void win(void) {
    puts("[win] you redirected control flow here.");
    system("/bin/sh");
}

#endif /* MED_HARNESS_H */
