// Isolates Bool boxed-method dispatch (INVOKE_BOOL_AND/OR/XOR/NOT/EQUALS).
// All five ops are pure value-class methods that lazy-rebox to raw bool;
// the JIT inline-emits each one as a single setcc. No allocation in the hot
// loop after warmup — purely measures the dispatch + cmp cost.
//
// TARGET: ~1-2s on dev machine. Adjust N if first run lands outside 1-5s.

import * from "../lib/primitives/Bool.mt";

int N = 1000000;
Bool t = new Bool(true);
Bool f = new Bool(false);

int trueHits = 0;
int eqHits = 0;

for (int i = 0; i < N; i = i + 1) {
    Bool a = new Bool(i % 2 == 0);
    Bool b = new Bool(i % 3 == 0);

    Bool combined = a.and(b).or(a.xor(b)).not();
    if (combined.getValue()) {
        trueHits = trueHits + 1;
    }
    if (a.equals(t)) {
        eqHits = eqHits + 1;
    }
    if (b.equals(f)) {
        eqHits = eqHits + 1;
    }
}

print("boxed_bool_dispatch_hot true=" + trueHits + " eq=" + eqHits);
