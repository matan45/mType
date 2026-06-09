// VK-1378 regression fixture: an instance method invoked via ScriptAPI /
// ScriptInterpreter, which routes through VirtualMachine::invokeMethod (NOT
// interpretLoop). A breakpoint inside this body must pause execution —
// proving the interop mini-loop honours the debug hook. Declares classes
// only; the callback parses/registers it, then drives it through interop.

class InteropBreakTarget {
    public int total;

    public constructor() {
        total = 0;
    }

    public function step(int delta): int {
        total = total + delta;
        return total;
    }

    public static function stepStatic(int delta): int {
        return delta + 1;
    }
}
