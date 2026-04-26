// MYT-211 regression: arithmetic operator with an inline function-call
// return value as the right operand. Before the fix, the JIT recorded the
// unboxed return-value virtreg as a hint slot AFTER the helper's cc.invoke
// — but asmjit's RA treats the cc.mov(Mem, val) store as val's last use and
// may free its physical register, so the immediate consumeGpHint reader in
// the next ADD_INT/SUB_INT/MUL_INT/DIV_INT got a stale virtreg.
//
// The bytecode shape that triggers the bug:
//   LOAD_LOCAL acc
//   LOAD_LOCAL c
//   CALL readValue
//   ADD_INT (or SUB_INT / MUL_INT / DIV_INT)
//   STORE_LOCAL acc
// Without an intermediate STORE_LOCAL the stale-virtreg hint goes straight
// into asmjit's arithmetic instruction.
//
// 1000 iterations per loop is enough to stabilize the IC and force JIT
// compilation, matching the discipline of get_field_cached_mono.mt.

class Counter {
    public int value;
    public constructor() {
        this.value = 7;
    }
}

function readValue(Counter c): int {
    return c.value;
}

Counter c = new Counter();

int accAdd = 0;
for (int i = 0; i < 1000; i = i + 1) {
    accAdd = accAdd + readValue(c);
}
print(accAdd);

int accSub = 100000;
for (int i = 0; i < 1000; i = i + 1) {
    accSub = accSub - readValue(c);
}
print(accSub);

int accMul = 1;
for (int i = 0; i < 10; i = i + 1) {
    accMul = accMul * readValue(c);
}
print(accMul);

int accDiv = 282475249;
for (int i = 0; i < 10; i = i + 1) {
    accDiv = accDiv / readValue(c);
}
print(accDiv);
