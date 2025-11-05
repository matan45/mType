// Pass test: Final interface can be defined
// Final interfaces just cannot be extended (but can be implemented)

final interface FinalInterface {
    function doWork(): void;
    function getValue(): int;
}

// Final interfaces can still be implemented by classes
class Worker implements FinalInterface {
    private int value;

    public constructor() {
        this.value = 10;
    }

    public function doWork(): void {
        this.value = this.value + 5;
    }

    public function getValue(): int {
        return this.value;
    }
}

Worker w = new Worker();
print(w.getValue());
w.doWork();
print(w.getValue());
