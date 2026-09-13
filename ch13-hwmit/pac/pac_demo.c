/* Build: aarch64-linux-gnu-gcc -O2 -march=armv8.5-a pac_demo.c -o pac_demo
 * Run:   qemu-aarch64 -cpu max ./pac_demo   (or on native ARMv8.3+ hw) */
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <sys/auxv.h>

static inline uint64_t pacia(uint64_t ptr, uint64_t mod) {
    asm("pacia %0, %1" : "+r"(ptr) : "r"(mod)); return ptr;   /* ❶ */
}
static inline uint64_t autia(uint64_t ptr, uint64_t mod) {
    asm("autia %0, %1" : "+r"(ptr) : "r"(mod)); return ptr;   /* ❷ */
}
static inline uint64_t xpaci(uint64_t ptr) {
    asm("xpaci %0"    : "+r"(ptr));            return ptr;    /* ❸ */
}

static int callee(int x) { return x * 2 + 1; }

int main(void) {
    unsigned long hw = getauxval(AT_HWCAP);
    printf("HWCAP_PACA = %lu\n", (hw >> 30) & 1);             /* ❹ */

    uint64_t raw    = (uint64_t)&callee;
    uint64_t signed_= pacia(raw, /*modifier=*/0xC0DEC0DE);    /* ❺ */
    printf("raw    = %016" PRIx64 "\n", raw);
    printf("signed = %016" PRIx64 "  (bits[54:48] hold the PAC)\n", signed_);

    /* Valid path: strip -> equals raw; auth with same modifier -> equals raw. */
    printf("xpaci  = %016" PRIx64 "\n", xpaci(signed_));
    uint64_t good   = autia(signed_, 0xC0DEC0DE);
    int (*fp_good)(int) = (int (*)(int))good;
    printf("good() = %d\n", fp_good(20));                     /* ❻ */

    /* Forge: flip a single signature bit and authenticate. */
    uint64_t forged = signed_ ^ (1ULL << 50);                 /* ❼ */
    uint64_t bad    = autia(forged, 0xC0DEC0DE);
    printf("forged after autia = %016" PRIx64 "\n", bad);
    int (*fp_bad)(int) = (int (*)(int))bad;
    return fp_bad(20);                                        /* ❽ SIGSEGV */
}
