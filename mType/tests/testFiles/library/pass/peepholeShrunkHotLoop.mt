// MYT-368 regression fixture. JIT codegen loops (JitCompiler_Frame and
// JitCompiler_Calls) walk [startOffset, startOffset + instructionCount)
// as the function's body. Before MYT-368, a stale count let the JIT
// emit code for instructions belonging to the NEXT function. This fixture
// places peephole-foldable arithmetic at the top of a hot-loop function
// so the body is shrunk by the optimizer, then exercises the loop
// 1000 times — enough iterations to drive the function warm but light
// enough not to slow down the suite.

function f(): int {
    int a = 1 + 2;
    int b = 3 * 4;
    int unused = a + b;
    int sum = 0;
    for (int i = 0; i < 1000; i = i + 1) {
        sum = sum + i;
    }
    return sum;
}

print(f());
