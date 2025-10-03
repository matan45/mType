// Test: Public fields accessible within same class
class Counter {
    public int count;

    public Counter(int initial) {
        count = initial;
    }

    public void increment() {
        count = count + 1;  // Accessing public field in same class
    }

    public int getCount() {
        return count;  // Accessing public field in same class
    }
}

Counter c = new Counter(5);
c.increment();
print(c.getCount());  // Expected: 6
