// Bool - Wrapper class for bool primitive
class Bool {
    public bool value;

    // Constructor
    public constructor(bool val) {
        this.value = val;
    }

    // Default constructor
    public constructor() {
        this.value = false;
    }

    // Get the primitive value
    public function getValue(): bool {
        return this.value;
    }

    // Set the primitive value
    public function setValue(bool val): void {
        this.value = val;
    }

    // Logical operations
    public function and(Bool other): Bool {
        return new Bool(this.value && other.value);
    }

    public function or(Bool other): Bool {
        return new Bool(this.value || other.value);
    }

    public function not(): Bool {
        return new Bool(!this.value);
    }

    // Comparison
    public function equals(Bool other): bool {
        return this.value == other.value;
    }

    // Utility
    public function toString(): string {
        return parsePrimitive(this.value);
    }

    public function hashCode(): int {
        return hashCode(this.value);
    }

    // Static factory methods
    public static function bool_TRUE(): Bool {
        return new Bool(true);
    }

    public static function bool_FALSE(): Bool {
        return new Bool(false);
    }
}