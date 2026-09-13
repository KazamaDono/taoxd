# Pointer compression, tag bits, and cross-engine cheatsheet

Reference sheet for the arithmetic you will bake into every V8 exploit
script for Chapters 21 and 22, plus the equivalents for the other two
mainstream engines.  Keep it next to your `d8` window.

## V8 (pinned 12.4, pointer compression + sandbox enabled)

### Tag bits (32-bit tagged word)

| Low bit | Meaning        | Payload                                |
|--------:|----------------|----------------------------------------|
| `0`     | **Smi**        | `value = word >> 1` (arithmetic shift) |
| `1`     | **HeapObject** | `compressed = word & ~1`               |

Smi range on a 32-bit tagged build: `[-2**30, 2**30 - 1]`.  Anything
outside becomes a `HeapNumber` on the managed heap.

### Cage-base arithmetic

```
cage_base = any_heap_addr & 0xFFFFFFFF00000000
real_addr = cage_base + (compressed & ~1)
compressed = (real_addr - cage_base) | 1        # for HeapObject tag
```

The cage base is per-isolate and per-run (ASLR randomises it).  In
generated code V8 keeps it in the register that
`src/codegen/<arch>/register-<arch>.h` names
`kPtrComprCageBaseRegister` — currently `r14` on x86-64 and `x28` on
AArch64 for the 12.4 tree, but check the header before you hard-code it.  Every two heap-object addresses printed by `%DebugPrint` share
the same upper 32 bits; that is the cage tag.

### Object header offsets (12.4, pointer-compressed build)

```
+0x00  map*         (compressed)
+0x04  properties*  (compressed)   -> PropertyArray or empty FixedArray
+0x08  elements*    (compressed)   -> FixedArray / FixedDoubleArray / etc.
+0x0C  in-object slot 0
+0x10  in-object slot 1
 ...
```

`FixedArray` and `FixedDoubleArray` both begin with `{map*, length}`
(two compressed words), then `length` element slots.

### ArrayBuffer / TypedArray

The illustrative order of fields, in the pinned 12.4 tree, is:

```
JSArrayBuffer:
  map*  |  properties*  |  elements*
  byte_length         (size_t, sandboxed / system-pointer sized)
  max_byte_length     (size_t)
  backing_store       (external-pointer-table handle, NOT a raw ptr)
  extension           (external)
  bit_field / flags

JSTypedArray (view):
  map*  |  properties*  |  elements*
  buffer*             (compressed ptr back into cage)
  byte_offset         (size_t)
  byte_length         (size_t)
  length              (size_t element count)
  external_pointer    (external-pointer-table handle)
  base_pointer        (Smi or nullptr)
```

The exact byte offsets, the widths of the size_t fields, and whether a
given field is compressed, sandboxed, or plain-external all shift
between V8 versions — always cross-check against
`src/objects/js-array-buffer.h` and `src/objects/js-array-buffer.tq`
in the pinned tree before you start counting bytes in a real exploit.

## JavaScriptCore (WebKit) — NaN-boxing

Every value is a 64-bit IEEE 754 double.  Non-double values live in the
"impure" NaN space (`exponent == 0x7ff` with a non-zero mantissa):

```
double v          : normal double bit pattern
int32 v           : 0xFFFF_0000_0000_0000 | (uint32)v
pointer p         : p                                (top 16 bits are 0
                                                      on x86-64 canonical)
false / true      : 0x06 / 0x07
null / undefined  : 0x02 / 0x0A
```

There is no cage; pointers are raw and 48-bit canonical.  A JSC
`addrof` primitive therefore hands you a real address, not an offset —
which is why the JSC exploit vocabulary predates and drove the V8 one.

`ArrayBuffer` backing stores live outside the JSC heap and are
referenced via `JSArrayBufferView`; the "external" boundary is softer
than V8's sandbox but the shape of the primitive (`byte_length` versus
raw pointer) is the same.

## SpiderMonkey (Firefox) — NaN-boxing, hardened externals

SpiderMonkey uses NaN-boxing similar to JSC.  Since Firefox 100+ the
`ArrayBuffer` backing-store pointer is mediated through an external
pointer table, mirroring the V8 hardening trajectory.  Shape identity
lives in `Shape` / `BaseShape`; the transition-tree intuition ports
directly.

## Reading V8 source when a dump and a blog disagree

The pins that matter for Chapter 20 in the 12.4 tree:

```
src/objects/js-objects.h        JSObject header & in-object slots
src/objects/map.h               Map, DescriptorArray, transitions
src/objects/fixed-array.h       FixedArray / FixedDoubleArray layouts
src/objects/js-array-buffer.h   JSArrayBuffer, JSTypedArray
src/sandbox/external-pointer-table.h
                                the external-pointer-table indirection
src/common/globals.h            kTaggedSize, kSystemPointerSize, cage
                                constants
```

When a value's `%DebugPrint` output surprises you, the ground truth is
in these headers.  Everything else — blogs, this book — is commentary.
