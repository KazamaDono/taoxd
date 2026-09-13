/* admin.c — seeded variant #1 of the network-tainted memcpy bug.
 *
 * The admin control plane reads a length prefix off a socket and hands it
 * to memcpy() without a bound against sizeof(dst).  The CodeQL taint
 * query from Listing 31-3 flags this exactly like the parent bug in
 * target/handler.c. */
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>

void admin_process(int fd)
{
    char pkt[1024];
    ssize_t n = recv(fd, pkt, sizeof(pkt), 0);    /* taint source */
    if (n < 8) return;

    unsigned int datalen;
    memcpy(&datalen, pkt + 4, sizeof(datalen));   /* length lives in bytes 4..7 */

    char dst[512];
    memcpy(dst, pkt + 8, datalen);                /* SINK — unbounded */
}
