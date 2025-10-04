// String - Wrapper class for string primitive
class String {
    public string value;

    // Constructor
    public constructor(string val) {
        value = val;
    }

    // Default constructor
    public constructor() {
        value = "";
    }

    // Get the primitive value
    public function getValue() : string {
        return value;
    }

    // Set the primitive value
    public function setValue(string val) : void {
        value = val;
    }

    // String operations
    public function length() : int {
        return strLength(value);
    }

    public function isEmpty() : bool {
        return value == "";
    }

    public function equals(String other) : bool {
        return value == other.value;
    }

    public function toString() : string {
        return this.value;
    }

    public function hashCode() : int {
        return hashCode(value);
    }

    // Concatenation
    public function concat(String other) : String {
        return new String(value + other.value);
    }
}