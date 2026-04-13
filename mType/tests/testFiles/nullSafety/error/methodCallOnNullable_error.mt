// Test: Calling a method on a nullable receiver should fail at compile time

class Counter {
    public int count;
    constructor(int c) {
        this.count = c;
    }
    public function increment(): void {
        this.count = this.count + 1;
    }
}

Counter? c = new Counter(0);
c.increment();
print("Should not reach here");
