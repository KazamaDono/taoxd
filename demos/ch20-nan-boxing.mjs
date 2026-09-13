// ch20: JS engine internals — this is a REAL Node.js session that shows
// how tagged small integers and doubles coexist in the same 64-bit
// value. V8 uses pointer compression (32-bit slots inside a 4-GiB cage);
// JavaScriptCore and SpiderMonkey historically use NaN-boxing (this
// demo). Both tricks solve the same "how do I fit both a pointer and a
// number in one machine word?" problem.
//
// Run:  node build/demo-scripts/ch20-nan-boxing.mjs

const buf = new ArrayBuffer(8);
const f64 = new Float64Array(buf);
const u32 = new Uint32Array(buf);

function bitsOf(x) {
  f64[0] = x;
  return (BigInt(u32[1]) << 32n) | BigInt(u32[0]);
}

function boxed(x) { return `0x${bitsOf(x).toString(16).padStart(16, '0')}`; }

console.log("=== NaN-boxing / tagged doubles (Ch 20 JS-engine internals) ===");
console.log();
console.log("A single 64-bit slot in a JS engine can hold:");
console.log(`  a real double        3.14              = ${boxed(3.14)}`);
console.log(`  a small integer       42                = ${boxed(42)}`);
console.log(`  positive infinity                       = ${boxed(Infinity)}`);
console.log(`  negative infinity                       = ${boxed(-Infinity)}`);
console.log(`  quiet NaN            NaN                = ${boxed(NaN)}`);
console.log(`  a tiny denormal      5e-324             = ${boxed(5e-324)}`);
console.log();

// NaN is special — the IEEE-754 quiet-NaN pattern has 51 unused bits.
// JSC hides tagged pointers and small ints in those bits.
console.log("Every IEEE-754 quiet NaN starts with 0x7ff8. That leaves 51 bits");
console.log("of payload the engine can use for a type tag + pointer + immediate.");
console.log();

// Show a few examples of what a JSC-flavoured tag looks like
const TAG_INT32   = 0xfffe000000000000n;
const TAG_UNDEF   = 0xfff8000000000000n;
const TAG_NULL    = 0xfff9000000000000n;
const TAG_BOOL    = 0xfffa000000000000n;

function withTag(tag, payload) {
  return `0x${(tag | BigInt(payload)).toString(16).padStart(16,'0')}`;
}
console.log("--- JSC-style boxed values ---");
console.log(`int32(42):         ${withTag(TAG_INT32, 42)}`);
console.log(`undefined:         0x${TAG_UNDEF.toString(16).padStart(16,'0')}`);
console.log(`null:              0x${TAG_NULL.toString(16).padStart(16,'0')}`);
console.log(`bool(true):        ${withTag(TAG_BOOL, 1)}`);
console.log();
console.log("The engine's runtime looks at the top 16 bits FIRST to decide");
console.log("whether to interpret the low 48 bits as a pointer, an int, or the");
console.log("raw bits of a double. Type-confusion exploits (Ch 21) get the");
console.log("compiler to skip that check.");
