// Test: Public fields accessible within same class
class Counter {
    public int count;

    constructor(int initial) {
        count = initial;
    }

    public function increment(): void {
        count = count + 1;  // Accessing public field in same class
    }

    public function getCount(): int {
        return count;  // Accessing public field in same class
    }
}

Counter c = new Counter(5);
c.increment();
print(c.getCount());  // Expected: 6
