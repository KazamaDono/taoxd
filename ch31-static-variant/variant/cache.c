/* cache.c — seeded variant #2 of the network-tainted memcpy bug.
 *
 * The cache-ingest path reads a fixed header off a file descriptor,
 * extracts a length from it, and copies the header's own trailing bytes
 * into a fixed-size body buffer.  Same shape as target/handler.c. */
#include <string.h>
#include <unistd.h>

void cache_ingest(int fd)
{
    char header[16];
    if (read(fd, header, sizeof(header)) != (ssize_t)sizeof(header))   /* taint source */
        return;

    unsigned int n;
    memcpy(&n, header + 8, sizeof(n));

    static char body[4096];
    memcpy(body, header, n);                       /* SINK — unbounded */
}
