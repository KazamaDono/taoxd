// excerpt: ch27-win-kernel/windbg/steal_token.js
// Usage: .scriptload steal_token.js ; dx @$scriptContents.dumpTokens()
"use strict";

function dumpTokens() {
    let out = [];
    for (let p of host.namespace.Debugger.State.PseudoRegisters
                     .General.processes.Iterator) {
        let ep = host.getModuleType("nt", "_EPROCESS")
                    .createTypedObject(p.KernelObject.address, "nt");
        let tokRef = ep.Token.Object.address;                 // ❶
        let tok = tokRef.bitwiseAnd(host.Int64(0xFFFFFFFFFFFFFFF0));
        out.push({ pid: p.Id,
                   name: p.Name,
                   token: tok.toString(16) });                // ❷
    }
    return out;
}
