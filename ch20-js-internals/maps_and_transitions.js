// d8 --allow-natives-syntax maps_and_transitions.js
//
// Walk the Map transition tree from three angles:
//  (1) two objects built {x, y} share a Map;
//  (2) an object built {y, x} lands on a DIFFERENT Map even though
//      the property set is identical (insertion order matters);
//  (3) `delete` forces the object into DICTIONARY mode, dropping its
//      DescriptorArray and severing the fast-path shape identity.

function build_xy() {
  const o = {};
  o.x = 1;
  o.y = 2;
  return o;
}

function build_yx() {
  const o = {};
  o.y = 2;
  o.x = 1;
  return o;
}

const xy1 = build_xy();
const xy2 = build_xy();
const yx  = build_yx();

print("--- {x,y} vs {x,y} ---");
%DebugPrint(xy1);
%DebugPrint(xy2);
print("xy1 same-map as xy2?", %HaveSameMap(xy1, xy2));   // true

print("--- {x,y} vs {y,x} ---");
%DebugPrint(yx);
print("xy1 same-map as yx?",  %HaveSameMap(xy1, yx));    // false

// Force dictionary-mode transition by adding many properties, then
// deleting one from the middle.  V8 gives up on the DescriptorArray
// and migrates the properties into a NameDictionary hash table.
print("--- dictionary transition ---");
const d = {};
for (let i = 0; i < 40; i++) d["k" + i] = i;
%DebugPrint(d);            // still FastProperties
delete d.k17;
%DebugPrint(d);            // now DictionaryProperties
print("dict same-map as fresh {x,y}?", %HaveSameMap(d, xy1));  // false
