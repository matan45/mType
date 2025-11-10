// Test exception handling in default parameter value
import * from "../../lib/exceptions/Exception.mt";

class ParamException extends Exception {
    constructor(string message): super(message) {
    }
}

function getDefaultValue(): int {
    try {
        print("Computing default parameter value");
        throw new ParamException("Error during default parameter computation");
    } catch (ParamException e) {
        print("Caught in default param: " + e.getMessage());
        return 50;
    }
}

function testFunction(int value = getDefaultValue()): void {
    print("Function called with value: " + value);
}

function main(): void {
    print("Testing exception in default parameter");
    testFunction();
    print("---");
    testFunction(75);
    print("Test completed");
}

main();
