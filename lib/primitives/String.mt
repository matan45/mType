// String - Wrapper class for string primitive
class String {
    string value;

    // Constructor
    constructor(string val) {
        value = val;
    }

    // Default constructor
    constructor() {
        value = "";
    }

    // Get the primitive value
    function getValue() : string {
        return value;
    }

    // Set the primitive value
    function setValue(string val) : void {
        value = val;
    }

    // String operations
    function length() : int {
        return strLength(value);
    }

    function isEmpty() : bool {
        return value == "";
    }

    function equals(String other) : bool {
        return value == other.value;
    }

    function toString() : string {
        return this.value;
    }

    function hashCode() : int {
        return hashCode(value);
    }

    // Concatenation
    function concat(String other) : String {
        return new String(value + other.value);
    }
}