// MYT-TBD: dedicated microbenchmark for box/unbox pressure on primitives.
// Existing boxed_* benchmarks bundle method dispatch + arithmetic +
// allocation in one tight loop and cannot isolate boxing cost. This one
// chases a single int through a Box<Int> wrapper every iteration: the
// allocation + generic-field T=Int load + value-class unwrap form a clean
// box/unbox pressure micro. Single-class hot loop keeps the IC MONO so the
// helper-call overhead in mType/vm/jit/JitCompiler_Boxing.cpp dominates the
// measurement rather than IC-miss noise. TARGET: ~1-3s wall time.
//
// JIT-correctness oracle (MYT-259): the .expected file is captured from
// --no-jit; the JIT path must produce byte-identical stdout.

import * from "../lib/primitives/Box.mt";
import * from "../lib/primitives/Int.mt";

int N = 2000000;
int sum = 0;
for (int i = 0; i < N; i = i + 1) {
    Box<Int> b = new Box<Int>(new Int(i));
    sum = sum + b.getValue().getValue();
}
print("box_unbox_hot sum=" + sum);
