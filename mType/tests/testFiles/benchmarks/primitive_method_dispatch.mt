// TARGET: ~2s on dev machine. Adjust N if first run lands outside 1-5s.
// Exercises INVOKE_INT_* / INVOKE_FLOAT_* specialized opcodes via the Int /
// Float wrapper classes. The compiler lowers these method calls to specialized
// opcodes (PrimitiveMethodOptimizer), which the interpreter handles but the
// JIT currently bails out of.
// Baseline coverage for MYT-144.

import * from "../lib/primitives/Int.mt";
import * from "../lib/primitives/Float.mt";

int N = 500000;

Int acc = new Int(0);
Int step = new Int(3);
Int mask = new Int(97);
for (int i = 0; i < N; i = i + 1) {
    Int cur = new Int(i);
    Int scaled = cur.multiply(step);
    Int shifted = scaled.add(acc);
    acc = shifted.modulo(mask);
}

Float facc = new Float(0.0);
Float fstep = new Float(1.1);
float fi = 0.0;
for (int i = 0; i < N; i = i + 1) {
    Float cur = new Float(fi);
    Float scaled = cur.multiply(fstep);
    facc = facc.add(scaled);
    fi = fi + 1.0;
}

print("primitive_method_dispatch accValue=" + acc.getValue() + " faccValue=" + facc.getValue());
