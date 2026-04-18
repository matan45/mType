// MYT-163 F-a: self-recursive method must not inline. The eligibility check
// rejects on nested CALL_METHOD (HAS_NESTED_CALL) and, if reached, on name
// match (SELF_RECURSIVE). This test guards correctness — any bad inlining
// would either overflow the stack or miscount.

class Counter {
    public function countDown(int n): int {
        if (n <= 0) {
            return 0;
        }
        return 1 + this.countDown(n - 1);
    }
}

Counter c = new Counter();
print(c.countDown(10));
print(c.countDown(25));
