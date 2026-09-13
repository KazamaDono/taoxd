/* ch23-sandbox/seccomp_policy.h */
#ifndef CH23_SECCOMP_POLICY_H
#define CH23_SECCOMP_POLICY_H

/* Installs the renderer's seccomp-bpf filter on the current thread.
 * Returns 0 on success, -1 on failure (errno set by prctl). */
int install_renderer_filter(void);

#endif
