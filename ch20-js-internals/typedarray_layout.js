// d8 --allow-natives-syntax typedarray_layout.js
const buf = new ArrayBuffer(0x1000);    // ❶ 4 KiB backing store
const u8  = new Uint8Array(buf);
u8[0] = 0x41;
u8[1] = 0x42;

%DebugPrint(u8);                        // ❷ view: byte_offset, byte_length, buffer*
%DebugPrint(buf);                       // ❸ ArrayBuffer: backing_store (external), length
