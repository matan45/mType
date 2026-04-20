// MYT-198: LOAD_LOCAL + CALL_METHOD_CACHED → LOAD_LOCAL_CALL_CACHED.
// Stable monomorphic dispatch where every iteration loads the same receiver
// from a local and calls a small method. After IC stabilization the opcode
// is rewritten to LOAD_LOCAL_CALL_CACHED via tryFuseWithPriorLoadLocal.
// Output must match the pre-fusion dispatch.

class Worker {
    public int total;
    public constructor() {
        this.total = 0;
    }
    public function record(int n): int {
        this.total = this.total + n;
        return this.total;
    }
}

Worker w = new Worker();
int acc = 0;
for (int i = 0; i < 1000; i = i + 1) {
    acc = acc + w.record(i);
}
print(acc);
print(w.total);
