// Test: Encapsulation with validation
class PositiveCounter {
    private int count;

    constructor() {
        count = 0;
    }

    public function increment(): void {
        count = count++;
    }

    public function decrement(): void {
        if (count > 0) {
            count = count--;
        }
    }

    public function getCount(): int {
        return count;
    }
}

PositiveCounter counter = new PositiveCounter();
print(counter.getCount());  // Expected: 0

counter.increment();
counter.increment();
counter.increment();
print(counter.getCount());  // Expected: 3

counter.decrement();
print(counter.getCount());  // Expected: 2
