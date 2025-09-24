// Float - Wrapper class for float primitive
class Float {
    float value;

    // Constructor
    constructor(float val) {
        this.value = val;
    }

    // Default constructor
    constructor() {
        this.value = 0.0;
    }

    // Get the primitive value
    function getValue(): float {
        return this.value;
    }

    // Set the primitive value
    function setValue(float val): void {
        this.value = val;
    }

    // Arithmetic operations
    function add(Float other): Float {
        return new Float(this.value + other.value);
    }

    function add(float other): Float {
        return new Float(this.value + other);
    }

    function subtract(Float other): Float {
        return new Float(this.value - other.value);
    }

    function multiply(Float other): Float {
        return new Float(this.value * other.value);
    }

    function divide(Float other): Float {
        return new Float(this.value / other.value);
    }

    // Comparison
    function equals(Float other): bool {
        return this.value == other.value;
    }

    function compareTo(Float other): int {
        if (this.value < other.value) return -1;
        if (this.value > other.value) return 1;
        return 0;
    }

    // Utility
    function toString(): string {
        return parsePrimitive(this.value);
    }

    function hashCode(): int {
        return hashCode(this.value);
    }

    function abs(): Float {
        if (this.value < 0.0) {
            return new Float(-this.value);
        }
        return new Float(this.value);
    }

    function floor(): Int {
        // This would need native implementation for floor operation
        return new Int(0); // Placeholder
    }

    function ceiling(): Int {
        // This would need native implementation for ceiling operation
        return new Int(0); // Placeholder
    }
}