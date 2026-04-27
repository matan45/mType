// MYT-226: a pure tail-recursive function whose body is `return f(...)` is
// lowered by the JIT to a tight back-edge (cc.jmp) and never pushes a
// CallFrame, so VirtualMachine::pushCallFrame's stack-overflow guard never
// fires. The program hangs forever instead of throwing "Stack overflow:".
//
// EXPECTED:
//   throws RuntimeException whose message starts with "Stack overflow:"
//   (matching the non-tail variant `return recurse(depth + 1) + 1;` which
//    DOES throw via the interpreter's pushCallFrame guard).
//
// ACTUAL (broken):
//   process hangs; no diagnostic, no termination.

function recurse(int depth): int {
    return recurse(depth + 1);
}

int result = recurse(0);
print(result);
