/* aslr_map.c - print the base address of each major region of our own
 * address space by reading /proc/self/maps.  Run it several times with
 * ASLR on to watch the bases change; run under `setarch -R ./aslr_map`
 * to watch them stay fixed.  Ubuntu 24.04. */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

int main(void)
{
    void *heap = malloc(1);                 /* force a [heap] mapping */

    FILE *f = fopen("/proc/self/maps", "r");
    if (!f) { perror("open maps"); return 1; }

    char line[512], perms[8], path[256];
    unsigned long long lo, hi;
    while (fgets(line, sizeof line, f)) {
        path[0] = '\0';
        if (sscanf(line, "%llx-%llx %7s %*x %*x:%*x %*u %255[^\n]",
                   &lo, &hi, perms, path) < 3)
            continue;

        const char *tag = NULL;
        if      (strstr(path, "aslr_map") && perms[2] == 'x') tag = "PIE  text ";
        else if (strstr(path, "libc.so.6") && perms[2] == 'x') tag = "libc text ";
        else if (strstr(path, "ld-linux")  && perms[2] == 'x') tag = "loader text";
        else if (strstr(path, "[heap]"))                       tag = "heap      ";
        else if (strstr(path, "[stack]"))                      tag = "stack     ";
        if (tag)
            printf("%s  base = %#018llx\n", tag, lo);
    }
    fclose(f);

    printf("a stack variable lives at %#018llx\n",
           (unsigned long long)(uintptr_t)&f);
    printf("the heap block lives at   %#018llx\n",
           (unsigned long long)(uintptr_t)heap);
    free(heap);
    return 0;
}
