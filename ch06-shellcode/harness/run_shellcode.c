/* run_shellcode.c — load a flat binary and execute it. Lab-only.
 * Build: gcc -O2 -o run_shellcode run_shellcode.c */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

int main(int argc, char **argv) {
    if (argc != 2) { fprintf(stderr, "usage: %s <blob>\n", argv[0]); return 2; }
    int fd = open(argv[1], O_RDONLY);
    if (fd < 0) { perror("open"); return 1; }
    long len = lseek(fd, 0, SEEK_END); lseek(fd, 0, SEEK_SET);

    void *page = mmap(NULL, len, PROT_READ | PROT_WRITE,        /* ❶ W, not X */
                      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (page == MAP_FAILED) { perror("mmap"); return 1; }
    if (read(fd, page, len) != len) { perror("read"); return 1; }
    close(fd);

    if (mprotect(page, len, PROT_READ | PROT_EXEC) != 0) {      /* ❷ flip to X */
        perror("mprotect"); return 1;
    }
    fprintf(stderr, "[*] executing %ld bytes at %p\n", len, page);
    ((void (*)(void))page)();                                   /* ❸ jump in */
    return 0;                                                    /* unreached */
}
