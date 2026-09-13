# Snapshot fuzzing walkthrough (Nyx-style, against the ch24 QEMU VM)

This directory is a **schematic** of the snapshot-fuzz loop from the chapter,
not a production snapshot fuzzer. The purpose is to see the pieces fit
together: the guest agent, the observer channel, and the entropy stubs. If
you need a real snapshot fuzzer, adopt [Nyx](https://nyx-fuzz.com/) upstream
rather than rewrite it.

## The loop, mapped to files

| Piece | Where it lives | What it does |
|-------|----------------|--------------|
| Guest agent | `agent.c` | Hypercalls the host for the next input, signals "done". |
| Snapshot boundary | ch24 QEMU VM from `code/ch24-*`, taken via `savevm` at the agent's first hypercall | Memory + vCPU + device state that we roll back per iteration. |
| Observer channel | I/O port `0x1337`, described in `agent.c` | Host writes input bytes into a shared page whose gpa the agent hands back on hypercall #1. |
| Entropy stubs | See `agent.c` comment block | Freeze the guest TSC at snapshot time; stub `RDRAND`/`RDSEED`; zero the allocator's fresh pages. |
| Coverage feedback | Intel-PT (host CPU) or a KVM breakpoint set derived from the driver's `.text` | Reported to the fuzzer between hypercall #1 (input placed) and hypercall #2 (done). |

## Running

Building `agent.c` needs only a hosted C toolchain — the interesting bits
are the inline `outb` sequences. `make -C ../  test` compiles it as part of
this chapter's CI smoke test. Actually driving a Nyx-style loop against the
ch24 VM requires the full Nyx toolchain (kAFL, QEMU-Nyx, libnyx); those
pins are outside this repo's CI budget.

## Warning

Everything about snapshot fuzzing amplifies non-determinism. Before you
chase a "flaky" crash: audit every source of entropy in the guest, freeze
the clock, and stub the RNG. See the chapter's warning box.
