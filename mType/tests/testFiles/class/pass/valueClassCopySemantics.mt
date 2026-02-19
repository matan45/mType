// Test: Value class copy semantics
// Expected: Pass - assignment creates independent copy

value class Counter {
    private int count;

    public constructor(int initial) {
        this.count = initial;
    }

    public function getCount(): int {
        return this.count;
    }

    public function increment(): Counter {
        return new Counter(this.count + 1);
    }

    public function toString(): string {
        return "Counter(" + this.count + ")";
    }
}

// Create and copy
Counter c1 = new Counter(10);
Counter c2 = c1;  // Should be a copy

print("c1: " + c1.toString());
print("c2: " + c2.toString());

// Modify c2 via new value - c1 should be unaffected
c2 = c2.increment();
print("After c2 increment:");
print("c1: " + c1.toString());
print("c2: " + c2.toString());

// Verify independence
print("c1 count: " + c1.getCount());
print("c2 count: " + c2.getCount());
