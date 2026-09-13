// d8 --allow-natives-syntax butterfly_probe.js
const arr = [1, 2, 3, 4];               // ❶ starts PACKED_SMI_ELEMENTS
%DebugPrint(arr);
const elems1 = %DebugPrint(arr);

for (let i = 4; i < 128; i++) arr[i] = i;  // ❷ crosses capacity; realloc
%DebugPrint(arr);

arr[64] = 1.5;                          // ❸ Smi -> Double kind transition
%DebugPrint(arr);                       //     backing store re-typed & copied
