// Error: `obj isClassOf T` in a non-generic context has no runtime binding
// for T to resolve to. The VM raises a clean RuntimeException naming the
// specific parameter that couldn't be resolved.

class Plain {
    public int n;
    public constructor(int n) { this.n = n; }

    public function check(Object o): bool {
        // T is not a declared parameter of Plain — this compiles (parser
        // accepts bare T as a type expression) but fails at run time when
        // INSTANCEOF_TYPEPARAM looks T up on the receiver.
        return o isClassOf T;
    }
}

Plain p = new Plain(1);
print(p.check(p));  // Should throw before reaching print
