# Contributing

Thanks for reading — and for wanting to help make this lab better.

The companion code has two audiences: readers running the labs, and researchers who spot a bug, want a technique demonstrated more clearly, or have a new chapter idea. This file covers both.

## Reporting a problem

Open an [issue](../../issues/new) with:

- **What you ran** (chapter, exact command, `uname -a` if it might matter),
- **What you expected** (usually: what the chapter says will happen),
- **What actually happened** (paste the output; screenshots are fine but text is easier to grep).

If the bug is in a running lab, tell us whether you're on the pinned Ubuntu 24.04 lab image (`./scripts/lab.sh`) or a different host. Ninety percent of "the offsets are wrong" reports turn out to be a host-vs-image mismatch.

::: warning
**Do not** open an issue that describes a technique against real, non-lab software. Report it to the vendor through their coordinated-disclosure channel (see the book's Appendix D). This repository is for the lab targets only.
:::

## Sending a fix

- Fork, branch, make the change, open a PR against `main`.
- **CI must stay green.** The [`labs`](.github/workflows/ci.yml) workflow builds every lab and runs every `make test` on Ubuntu 24.04 x86-64 and arm64. If your change breaks a test, either fix the test or explain why the change makes it correctly fail in the PR description.
- Match the local style. C follows the surrounding file's brace/indent. Python is 4 spaces, `black`-compatible. Shell is `bash`, `set -euo pipefail`.
- Keep chapter directories self-contained. If two chapters would share code, put it in `common/` and add a brief `common/README.md` line.
- If your change updates a code listing that appears in the book, note the chapter and listing id in the PR (`Chapter 8, Listing 8-3`) so the manuscript can be updated in lockstep.

## Adding a new lab or improving an existing one

The bar is: *reproducible, minimal, and teaching-focused*. A lab should:

- Illustrate one clear idea. If you're teaching two things, that's two labs.
- Ship with a `Makefile` that has a working `test` target — a deterministic exit-0-on-success check, not "run it and look."
- Annotate every deliberate weakness (a `-fno-stack-protector` flag, an unchecked read) in a comment naming *why* it's there.
- Include a paragraph in the chapter directory's `README.md` naming the file, what it demonstrates, and the expected `make test` output.

If a lab genuinely can't be exercised by the Linux CI (Windows-only, real hardware needed for MTE bare-metal), add `.ci-skip` with a one-line reason and provide a build/run script plus a note in the README.

## Reporting a security issue *in this repository's own code*

Everything here is intentionally vulnerable — that's not a security issue.

If you find a way for a lab to *escape its container onto a reader's host*, or a place where a build step downloads and runs untrusted code without pinning, please report it privately: open a GitHub Security Advisory instead of an issue. That is a real vulnerability in the teaching platform and we want to fix it before it hits readers.

## Code of conduct

Be excellent to each other. Technical criticism of a PR is welcome and expected; personal attacks and harassment are not. If you would not say something in person in front of your grandmother, don't put it in an issue thread.
