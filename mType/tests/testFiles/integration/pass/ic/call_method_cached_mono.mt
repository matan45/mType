// MYT-173: CALL_METHOD_CACHED correctness under a stable monomorphic call site.
// Runs enough iterations that the IC stabilizes to MONO and the bytecode
// rewriter promotes the opcode to CALL_METHOD_CACHED. Output must match the
// pre-rewrite generic CALL_METHOD dispatch.

class Counter {
    public int value;
    public function Counter() {
        this.value = 0;
    }
    public function tick(int step): int {
        this.value = this.value + step;
        return this.value;
    }
}

Counter c = new Counter();
int acc = 0;
for (int i = 0; i < 1000; i = i + 1) {
    acc = acc + c.tick(i);
}
print(acc);
print(c.value);
