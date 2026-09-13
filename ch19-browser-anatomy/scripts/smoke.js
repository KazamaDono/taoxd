// Run: ./d8 --allow-natives-syntax scripts/smoke.js
// Expect: DebugPrint output and a pointer-compression banner.

const a = [1.1, 2.2, 3.3];               // ❶ PACKED_DOUBLE_ELEMENTS array
%DebugPrint(a);                          // ❷ V8 dumps map, elements, length

function hot(x) { return x + 1; }
for (let i = 0; i < 100000; i++) hot(i); // ❸ warm to TurboFan
%OptimizeFunctionOnNextCall(hot);
hot(1);
%DebugPrint(hot);                        // ❹ shows optimized code entry

// Pointer compression sanity: on a compressed build, %DebugPrint prints
// short (32-bit) addresses inside the cage. On an uncompressed build,
// full 64-bit pointers appear. The exercises in Ch. 20 assume compressed.
print("d8 ready. If you saw two DebugPrint blocks, Part V will run.");
