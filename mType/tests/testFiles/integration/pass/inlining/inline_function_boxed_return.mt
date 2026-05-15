// MYT-316: plain-CALL inlining with a non-primitive (boxed) return type.
// tryEmitInlinedStaticCall (CALL_STATIC) rejects non-primitive callees;
// the new shared tryEmitInlinedFunctionCall wired into emitCallOp /
// emitCallFastOp accepts them. Output correctness oracle: the inlined
// path must produce the same string the runtime-dispatched path does.

import * from "../../../lib/primitives/String.mt";

function greet(int n): String {
    return "hi-" + n;
}

print(greet(1));   // hi-1
print(greet(2));   // hi-2
print(greet(42));  // hi-42
