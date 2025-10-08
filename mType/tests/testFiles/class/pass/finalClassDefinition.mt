// Pass test: Final class can be defined and used
// Final classes just cannot be extended

final class FinalClass {
    int value;

    constructor(int v) {
        value = v;
    }

    public function getValue(): int {
        return value;
    }

    public function setValue(int v): void {
        value = v;
    }
}

// Final classes can be instantiated
FinalClass obj = new FinalClass(42);
print(obj.getValue());

obj.setValue(100);
print(obj.getValue());
