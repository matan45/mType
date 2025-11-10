// Test exception handling in field initializer
import * from "../../lib/exceptions/Exception.mt";

class FieldException extends Exception {
    constructor(string message): super(message) {
    }
}

class TestClass {
    int field;
	
	constructor(){
		field = computeInitialValue();
	}

    public function computeInitialValue(): int {
        try {
            print("Computing field initial value");
            throw new FieldException("Error during field initialization");
        } catch (FieldException e) {
            print("Caught in field init: " + e.getMessage());
            return 100;
        }
    }

    public function getField(): int {
        return this.field;
    }
}

function main(): void {
    print("Testing exception in field initializer");
    TestClass obj = new TestClass();
    print("Field value: " + obj.getField());
    print("Test completed");
}

main();
