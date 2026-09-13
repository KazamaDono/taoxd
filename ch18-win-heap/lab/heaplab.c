/* excerpt — see ch18-win-heap/lab/heaplab.c for the full driver.
 * Build: cl /Zi /Od /MT heaplab.c /link /SUBSYSTEM:CONSOLE
 * Run:   heaplab.exe prime 0x40 64        (bucket, size, count)          */
#include <windows.h>
#include <stdio.h>

static HANDLE g_heap;                                                  /* ❶ */

static void prime_bucket(SIZE_T sz, unsigned count)
{
    void **p = calloc(count, sizeof(void *));
    for (unsigned i = 0; i < count; i++)
        p[i] = HeapAlloc(g_heap, 0, sz);                               /* ❷ */
    printf("[+] primed bucket for size=0x%zx, first=%p last=%p\n",
           sz, p[0], p[count - 1]);
    for (unsigned i = 0; i < count; i++)
        HeapFree(g_heap, 0, p[i]);                                     /* ❸ */
    free(p);
}

int main(int argc, char **argv)
{
    /* Opt this process in to the Segment Heap: the manifest embedded
     * by the linker flag /MANIFESTINPUT sets <heapType>SegmentHeap.
     * Confirm with: !heap -s in WinDbg -> "SegmentHeap".              ❹ */
    g_heap = GetProcessHeap();
    if (argc >= 4 && strcmp(argv[1], "prime") == 0)
        prime_bucket(strtoull(argv[2], NULL, 0), atoi(argv[3]));

    puts("[+] sleeping so you can attach WinDbg; press enter to exit.");
    getchar();
    return 0;
}
