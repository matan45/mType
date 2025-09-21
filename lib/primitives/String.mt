// String - Wrapper class for string primitive
class String {
    string value;

    // Constructor
    constructor(string val) {
        this.value = val;
    }

    // Default constructor
    constructor() {
        this.value = "";
    }

    // Get the primitive value
    function getValue(): string {
        return this.value;
    }

    // Set the primitive value
    function setValue(string val): void {
        this.value = val;
    }

    // String operations
    function length(): int {
        // This would need native implementation
        return 0; // Placeholder
    }

    function isEmpty(): bool {
        return this.value == "";
    }

    function equals(String other): bool {
        return this.value == other.value;
    }

    function toString(): string {
        return this.value;
    }

    function hashCode(): int {
        return hashCode(this.value);
    }

    // Concatenation
    function concat(String other): String {
        return new String(this.value + other.value);
    }
}