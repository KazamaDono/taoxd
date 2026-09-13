/* args8.c - a function with eight integer parameters, so the ABI has to
 * spill the extras on x86-64 (six arg registers) but not on AArch64
 * (eight arg registers).  Build: make ; make arm */
long pack(long a, long b, long c, long d,
          long e, long f, long g, long h)
{
    return a ^ (b << 1) ^ (c << 2) ^ (d << 3)
             ^ (e << 4) ^ (f << 5) ^ (g << 6) ^ (h << 7);
}

int main(void)
{
    return (int) pack(1, 2, 3, 4, 5, 6, 7, 8);
}
