// Int - Wrapper class for int primitive
class Int {
    int value;

    // Constructor
    constructor(int val) {
        this.value = val;
    }

    // Default constructor
    constructor() {
        this.value = 0;
    }

    // Get the primitive value
    function getValue(): int {
        return this.value;
    }

    // Set the primitive value
    function setValue(int val): void {
        this.value = val;
    }

    // Arithmetic operations
    function add(Int other): Int {
        return new Int(this.value + other.value);
    }

    function add(int other): Int {
        return new Int(this.value + other);
    }

    function subtract(Int other): Int {
        return new Int(this.value - other.value);
    }

    function multiply(Int other): Int {
        return new Int(this.value * other.value);
    }

    function divide(Int other): Int {
        return new Int(this.value / other.value);
    }

    // Comparison
    function equals(Int other): bool {
        return this.value == other.value;
    }

    function compareTo(Int other): int {
        if (this.value < other.value) return -1;
        if (this.value > other.value) return 1;
        return 0;
    }

    // Utility
    function toString(): string {
        return toString(this.value);
    }

    function hashCode(): int {
        return hashCode(this.value);
    }

    function abs(): Int {
        if (this.value < 0) {
            return new Int(-this.value);
        }
        return new Int(this.value);
    }
}