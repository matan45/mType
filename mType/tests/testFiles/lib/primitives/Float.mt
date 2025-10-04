// Float - Wrapper class for float primitive
class Float {
    public float value;

    // Constructor
    public constructor(float val) {
        this.value = val;
    }

    // Default constructor
    public constructor() {
        this.value = 0.0;
    }

    // Get the primitive value
    public function getValue(): float {
        return this.value;
    }

    // Set the primitive value
    public function setValue(float val): void {
        this.value = val;
    }

    // Arithmetic operations
    public function add(Float other): Float {
        return new Float(this.value + other.value);
    }

    public function add(float other): Float {
        return new Float(this.value + other);
    }

    public function subtract(Float other): Float {
        return new Float(this.value - other.value);
    }

    public function multiply(Float other): Float {
        return new Float(this.value * other.value);
    }

    public function divide(Float other): Float {
        return new Float(this.value / other.value);
    }

    // Comparison
    public function equals(Float other): bool {
        return this.value == other.value;
    }

    public function compareTo(Float other): int {
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

    public function abs(): Float {
        if (this.value < 0.0) {
            return new Float(-this.value);
        }
        return new Float(this.value);
    }

    public function floor(): Int {
        // This would need native implementation for floor operation
        return new Int(0); // Placeholder
    }

    public function ceiling(): Int {
        // This would need native implementation for ceiling operation
        return new Int(0); // Placeholder
    }
}