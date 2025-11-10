// Test: Casting in while loop condition
class Counter {
    public int count;

    constructor(int c) {
        this.count = c;
    }

    public function decrement(): void {
        this.count = this.count - 1;
    }

    public function getValue(): int {
        return this.count;
    }
}

class AdvancedCounter extends Counter {
    public int step;

    constructor(int c, int s):super(c) {
        this.step = s;
    }

    public function decrementByStep(): void {
        this.count = this.count - this.step;
    }
}

// Cast in while condition
Counter obj = new AdvancedCounter(10, 2);
while (((AdvancedCounter)obj).getValue() > 0) {
    print(((AdvancedCounter)obj).getValue());
    ((AdvancedCounter)obj).decrementByStep();
}

print("Done");

// Expected output:
// 10
// 8
// 6
// 4
// 2
// Done
