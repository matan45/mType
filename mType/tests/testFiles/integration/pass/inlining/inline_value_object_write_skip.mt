// MYT-167 F-e: value-class receivers with field-writing methods must NOT be
// inlined — eligibility returns VALUE_OBJECT_WRITES_FIELDS. The call falls
// through to jit_call_method's ValueObject materialisation branch, which
// creates a temp ObjectInstance per call and discards it after; caller's
// ValueObject stays unmutated. Each call therefore returns 1 (bumps 0 → 1).

value class Counter {
    public int n;

    public constructor(int n) {
        this.n = n;
    }

    public function bumpAndGet(): int {
        this.n = this.n + 1;
        return this.n;
    }
}

Counter c = new Counter(0);
int total = 0;
for (int i = 0; i < 200; i = i + 1) {
    total = total + c.bumpAndGet();
}
print(total);
