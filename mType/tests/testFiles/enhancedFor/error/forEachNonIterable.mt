// Test: For-each loop on non-Iterable type should fail at compile time
// Expected: Type error - class does not implement Iterable<T>

class NonIterableClass {
    private int value;

    constructor(int val) {
        this.value = val;
    }

    public function getValue(): int {
        return this.value;
    }
}

NonIterableClass obj = new NonIterableClass(42);

// This should fail: NonIterableClass doesn't implement Iterable<T>
for (int x : obj) {
    print(x);
}
