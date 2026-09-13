#!/usr/bin/env python3
"""ch13: ARM PAC — how a pointer authentication code lives in the top
bits of a 64-bit pointer, and what "forge and fault" looks like when
the CPU's autia instruction fails to verify."""

def pac_layout(ptr_low_48: int, sig_16: int) -> int:
    """PAC 'authentic' pointer: top 16 bits hold the signature, bottom
    48 bits are the actual VA. Real hardware also uses a bit to mark
    kernel vs user pointers; ignored here for clarity."""
    assert ptr_low_48 & 0xFFFF_0000_0000_0000 == 0, "48-bit VA only"
    return (sig_16 << 48) | ptr_low_48

def strip(auth_ptr: int) -> int:
    """xpaci-style strip: return the underlying VA."""
    return auth_ptr & 0x0000_FFFF_FFFF_FFFF

# A real signed return address (as if produced by paciasp on entry)
va         = 0x0000_ffff_8020_1234        # the address _we return to
key_a_sig  = 0xa3b7                        # PAC-key A signature (16 bits)
signed_ra  = pac_layout(va, key_a_sig)

print("=== ARM pointer authentication (PAC) — the anatomy of a signed pointer ===")
print(f"raw VA (48-bit):     {va:#018x}")
print(f"PAC signature:       {key_a_sig:#06x}  (16-bit MAC over VA + modifier + key)")
print(f"signed pointer:      {signed_ra:#018x}")
print()
print("bit layout (little end -> big):")
print(f"  [63..48]  signature   = {(signed_ra >> 48) & 0xffff:016b}   ({(signed_ra>>48)&0xffff:#06x})")
print(f"  [47..0]   virtual addr= {va:048b}   ({va:#014x})")
print()
print("--- what happens when the attacker overwrites the saved LR ---")
attacker_target = 0x0000_ffff_deadbeef      # where the attacker wants control
naive_write     = attacker_target            # they just write raw VA
autia_result    = strip(naive_write) if (naive_write >> 48) == key_a_sig else None
print(f"attacker writes:     {naive_write:#018x}  (raw VA, no signature)")
print(f"CPU 'autia' checks:  top 16 bits vs recomputed MAC")
print(f"  written top bits:  {(naive_write>>48) & 0xffff:#06x}")
print(f"  expected top bits: {key_a_sig:#06x}")
print(f"  match?             {'yes' if (naive_write>>48)==key_a_sig else 'NO'}")
print()
if (naive_write>>48) != key_a_sig:
    print("  autia FAILS -> pointer gets a poison bit set")
    poisoned = naive_write | (1 << 55)  # ARM sets a fault-provoking bit
    print(f"  poisoned pointer:  {poisoned:#018x}")
    print(f"  ret branches to:   {poisoned:#018x}  ->  translation fault (SIGSEGV)")
print()
print("attacker's remaining options (Ch. 13 discusses each):")
print("  - reuse an already-signed pointer (signing-gadget attacks)")
print("  - forge across contexts if the modifier is guessable")
print("  - PACMAN (ISCA 2022): speculatively bypass the check via a side channel")
