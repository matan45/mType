// Pass test: Final interface can be defined
// Final interfaces just cannot be extended (but can be implemented)

final interface FinalInterface {
    function doWork(): void;
    function getValue(): int;
}

// Final interfaces can still be implemented by classes
class Worker implements FinalInterface {
    int value;

    constructor() {
        value = 10;
    }

    public function doWork(): void {
        value = value + 5;
    }

    public function getValue(): int {
        return value;
    }
}

Worker w = new Worker();
print(w.getValue());
w.doWork();
print(w.getValue());
