// Test exception handling in static initializer block
import * from "../../lib/exceptions/Exception.mt";

class InitException extends Exception {
    constructor(String message) {
        super(message);
    }
}

class TestClass {
    static Int staticField = getInitValue();

    static function getInitValue(): Int {
        try {
            print("Initializing static field");
            throw new InitException("Error during static initialization");
        } catch (InitException e) {
            print("Caught in static init: " + e.getMessage());
            return 42;
        }
    }
}

function main(): void {
    print("Testing exception in static initializer");
    Int value = TestClass.staticField;
    print("Static field value: " + value);
    print("Test completed");
}

main();
