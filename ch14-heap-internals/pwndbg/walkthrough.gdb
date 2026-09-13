# ch14-heap-internals/pwndbg/walkthrough.gdb
#
# Reproduces the pwndbg session shown in Listing 14-4 of Chapter 14.
# Requires the ../playground/playground binary and pwndbg loaded from
# ~/.gdbinit (the Ubuntu 24.04 lab image ships it that way).
#
# Usage (from ch14-heap-internals/pwndbg):
#   gdb -x walkthrough.gdb ../playground/playground
#
# The script feeds the same command sequence as ../playground/script.in
# via `set args`-driven stdin, breaks after the third free(), and issues
# heap / tcachebins / vis_heap_chunks so the reader sees the same three
# views the book prints.

set pagination off
set confirm off

# Break right after the third free() lands in the tcache. In script.in the
# three frees are consecutive; the fourth line after them is `tcache`, so
# any of these calls make a good stop.
# We break on the entry to __libc_free's third invocation by counting hits.
break free
commands
  silent
  set $hits = $hits + 1
  if $hits == 3
    printf "\n== stopped after third free() ==\n"
    # pwndbg commands:
    heap
    tcachebins
    vis_heap_chunks
    detach
    quit
  end
  continue
end

set $hits = 0

# Feed the same script the Makefile uses; pipe it in via a here-doc file.
# gdb can't read stdin scripts directly, so we use `run` with a redirect.
run < ../playground/script.in
