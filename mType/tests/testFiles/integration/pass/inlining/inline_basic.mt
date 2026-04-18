// MYT-163 F-a: trivial monomorphic method call. Verifies the inlined result
// matches the non-inlined one. The method body is 3 ops (LOAD_LOCAL, MUL_INT
// constant, RETURN_VALUE) — inside F-a's size limit with no internal jumps.

class A {
    public function compute(int x): int {
        return x * 2;
    }
}

A a = new A();
print(a.compute(5));
print(a.compute(10));
print(a.compute(21));
