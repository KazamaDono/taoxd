// Run with: d8 --allow-natives-syntax object_layout.js
function make() {
  const o = {};        // ❶ start at the empty-object Map
  o.x = 1;             // ❷ Map transition: add x as Smi field 0
  o.y = 2;             // ❸ Map transition: add y as Smi field 1
  return o;
}

const a = make();
const b = make();

%DebugPrint(a);        // ❹ dump a's Map, properties, elements
%DebugPrint(b);
print("same map:", %HaveSameMap(a, b));   // ❺ expect: true
