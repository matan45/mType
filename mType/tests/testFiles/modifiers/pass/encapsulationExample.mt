// Test: Encapsulation with validation
class PositiveCounter {
    private int count;

    public PositiveCounter() {
        count = 0;
    }

    public void increment() {
        count = count + 1;
    }

    public void decrement() {
        if (count > 0) {
            count = count - 1;
        }
    }

    public int getCount() {
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
