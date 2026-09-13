'use strict';
// Usage: frida -f ./crackme-x86_64 -l hook.js -- ANY-WRONG-KEY-HERE
const MODNAME = 'crackme-x86_64';

function resolveCheckSerial() {
  const sym = DebugSymbol.fromName('check_serial');        // ❶ needs .symtab
  if (sym && !sym.address.isNull()) return sym.address;

  const mod = Process.getModuleByName(MODNAME);            // ❷ stripped fallback
  return mod.base.add(0x11a9);                             // illustrative offset
}

Interceptor.attach(resolveCheckSerial(), {
  onEnter(args) {
    this.key = args[0].readUtf8String();                   // ❸ arg0 = the key
    console.log(`[check_serial] candidate = ${JSON.stringify(this.key)}`);
  },
  onLeave(retval) {
    console.log(`[check_serial] real return = ${retval.toInt32()}`);
    retval.replace(1);                                     // ❹ force "valid"
  }
});
