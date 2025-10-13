// Int - Wrapper class for int primitive
class Int {
    public int value;

    // Constructor
    public constructor(int val) {
        this.value = val;
    }

    // Default constructor
    public constructor() {
        this.value = 0;
    }

    // Get the primitive value
    public function getValue(): int {
        return this.value;
    }

    // Set the primitive value
    public function setValue(int val): void {
        this.value = val;
    }

    public function add(int other): Int {
        return new Int(this.value + other);
    }

    public function subtract(Int other): Int {
        return new Int(this.value - other.value);
    }

    public function multiply(Int other): Int {
        return new Int(this.value * other.value);
    }

    public function divide(Int other): Int {
        return new Int(this.value / other.value);
    }

    // Comparison
    public function equals(Int other): bool {
        return this.value == other.value;
    }

    public function compareTo(Int other): int {
        if (this.value < other.value) return -1;
        if (this.value > other.value) return 1;
        return 0;
    }

    // Utility
    public function toString(): string {
        return parsePrimitive(this.value);
    }

    public function hashCode(): int {
        return hashCode(this.value);
    }

    public function abs(): Int {
        if (this.value < 0) {
            return new Int(-this.value);
        }
        return new Int(this.value);
    }
}