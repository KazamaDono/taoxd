# Find functions that call a length routine and a comparison routine,
# then print their decompilation. Ghidra 11.x, run from the Script Manager.
# @category CH04
from ghidra.app.decompiler import DecompInterface
from ghidra.util.task import ConsoleTaskMonitor

LEN_FUNCS = ("strlen", "strnlen")                         # ❶
CMP_FUNCS = ("memcmp", "strcmp", "strncmp", "bcmp")

def called_names(func):
    """Names of functions this function calls (follows thunks)."""
    out = set()
    for callee in func.getCalledFunctions(ConsoleTaskMonitor()):  # ❷
        c = callee
        while c.isThunk():                                # ❸ resolve PLT thunks
            c = c.getThunkedFunction(True)
        out.add(c.getName())
    return out

def decompile(ifc, func):
    res = ifc.decompileFunction(func, 60, ConsoleTaskMonitor())
    return res.getDecompiledFunction().getC() if res.decompileCompleted() else None

ifc = DecompInterface()
ifc.openProgram(currentProgram)                           # ❹ provided by Ghidra

fm = currentProgram.getFunctionManager()
for func in fm.getFunctions(True):                        # True = forward order
    names = called_names(func)
    if names & set(LEN_FUNCS) and names & set(CMP_FUNCS): # ❺ calls both kinds
        print("=" * 60)
        print("candidate comparator: %s @ %s"
              % (func.getName(), func.getEntryPoint()))
        src = decompile(ifc, func)
        if src:
            print(src)
