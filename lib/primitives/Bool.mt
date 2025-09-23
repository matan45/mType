// Bool - Wrapper class for bool primitive
class Bool {
    bool value;

    // Constructor
    constructor(bool val) {
        this.value = val;
    }

    // Default constructor
    constructor() {
        this.value = false;
    }

    // Get the primitive value
    function getValue(): bool {
        return this.value;
    }

    // Set the primitive value
    function setValue(bool val): void {
        this.value = val;
    }

    // Logical operations
    function and(Bool other): Bool {
        return new Bool(this.value && other.value);
    }

    function or(Bool other): Bool {
        return new Bool(this.value || other.value);
    }

    function not(): Bool {
        return new Bool(!this.value);
    }

    // Comparison
    function equals(Bool other): bool {
        return this.value == other.value;
    }

    // Utility
    function toString(): string {
        return parsePrimitive(this.value);
    }

    function hashCode(): int {
        return hashCode(this.value);
    }

    // Static factory methods
    static function TRUE(): Bool {
        return new Bool(true);
    }

    static function FALSE(): Bool {
        return new Bool(false);
    }
}