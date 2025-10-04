// Test: Encapsulation with validation
class PositiveCounter {
    private int count;

    constructor() {
        count = 0;
    }

    public function increment(): void {
        this.count++;
    }

    public function decrement(): void {
        if (this.count > 0) {
            this.count--;
        }
    }

    public function getCount(): int {
        return this.count;
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
