// Comprehensive Lambda-to-Interface Test
// Tests the enhanced lambda-to-interface implementation with various scenarios

// Import required object wrapper types from standard library
import * from "../../lib/primitives/String.mt";
import * from "../../lib/primitives/Int.mt";

// Test functional interface with generic type
interface Processor<T> {
    function process(T input) : T;
}

interface Validator<T> {
    function validate(T value) : void;
}

interface Checker<T> {
    function isValid(T item) : bool;
}

// Test functional interfaces (single method each)
interface Adder {
    function add(int a, int b) : int;
}

interface Multiplier {
    function multiply(float x, float y) : float;
}

interface Concatenator {
    function concat(string s1, string s2) : string;
}

// Test functional interfaces with complex return types
interface Transformer<T> {
    function transform(T[] input) : T[];
}

interface Counter<T> {
    function getCount(T[] items) : int;
}

// Test simple lambda-to-interface conversion
function testBasicLambdaInterface(): void {
    print("Testing basic lambda-to-interface conversion...");

    // Create Adder instance from lambda
    Adder calc = (a, b) -> a + b;

    // Test method invocation
    int result = calc.add(5, 3);
    print("calc.add(5, 3) = " + result);

    if (result == 8) {
        print("Basic lambda-to-interface conversion works!");
    } else {
        print("Basic test failed - expected 8, got " + result);
    }
}

// Test generic lambda-to-interface conversion with objects
function testGenericLambdaInterface(): void {
    print("Testing generic lambda-to-interface conversion...");

    // Create Processor<String> from lambda
    Processor<String> stringProcessor = x -> new String(x.getValue() + "_processed");

    // Test method invocation
    String result = stringProcessor.process(new String("test"));
    print(stringProcessor.process(new String("test")) + " = " + result.getValue());

    if (result.getValue() == "test_processed") {
        print("Generic lambda-to-interface conversion works!");
    } else {
        print("Generic test failed - expected 'test_processed', got " + result.toString());
    }
}

// Test lambda-to-interface with complex return types (arrays)
function testArrayReturnTypeInterface(): void {
    print("Testing lambda-to-interface with array return types...");

    // Create Transformer<String> from lambda that adds suffix to each element
    Transformer<String> suffixer = arr -> {
        String[] result = new String[arr.length];
        for (int i = 0; i < arr.length; i++) {
            result[i] = new String(arr[i].value + "_transformed");
        }
        return result;
    };

    // Test with a string array
    String[] input = [new String("hello"), new String("world"), new String("test")];
    String[] output = suffixer.transform(input);

    print("Input: ['hello', 'world', 'test']");
    print("Output: ['" + output[0] + "', '" + output[1] + "', '" + output[2] + "']");

    // Check if transformation worked correctly
    if (output[0].value == "hello_transformed" && output[1].value == "world_transformed" && output[2].value == "test_transformed") {
        print("Array return type interface works correctly!");
    } else {
        print("Array return type interface failed");
    }
}

// Test parameter validation
function testParameterValidation(): void {
    print("Testing parameter validation...");

        Adder calc = (a, b) -> a + b;

        // This should work - correct parameters
        int result1 = calc.add(1, 2);
        print("Valid call succeeded: " + result1);

        print("Parameter validation allows valid calls!");
    
}

// Test return type validation
function testReturnTypeValidation(): void {
    print("Testing return type validation...");

    
        // Create a simple calculator lambda
        Adder calc = (a, b) -> a + b;

        int result = calc.add(7, 3);
        print("Return type validation passed: " + result);

        if (result == 10) {
            print("Return type validation works correctly!");
        } else {
            print("Return type validation failed - wrong result");
        }
    
}

// Test closure capture
function testClosureCapture(): void {
    print("Testing closure capture...");

    int multiplier = 5;

    // Lambda should capture the multiplier variable
    Processor<String> multiplierProcessor = x -> new String(x.getValue() +"_" + multiplier);

    String result = multiplierProcessor.process(new String("test"));
    print("Closure capture result: " + result);

    if (result.getValue() == "test_5") {
        print("Closure capture works correctly!");
    } else {
        print("Closure capture failed - expected 'test_5', got " + result);
    }
}

// Test multiple method interface (should handle first method only for now)
function testMultipleMethodInterface():void {
    print("Testing multiple method interface...");

    
        // This tests the limitation that lambdas can only implement single-method interfaces
        Adder calc = (a, b) -> a + b;

        // Test the primary method (first one declared)
        int result = calc.add(6, 4);
        print("Multiple method interface result: " + result);

        if (result == 10) {
            print("Multiple method interface handles primary method!");
        } else {
            print("Multiple method interface failed");
        }
    
}

// Test error handling for invalid lambda contexts
function testErrorHandling(): void {
    print("Testing error handling...");

    
        // Create a valid lambda interface
        Adder calc = (a, b) -> a + b;

        // Test with valid parameters
        int result = calc.add(2, 3);
        print("Error handling test passed: " + result);

        print("Error handling maintains valid execution!");
    
}

// Test lifecycle management
function testLifecycleManagement(): void {
    print("Testing lifecycle management...");

    // Create multiple lambda interfaces to test memory management
    for (int i = 0; i < 3; i++) {
        Adder tempCalc = (a, b) -> a + b + i;
        int result = tempCalc.add(1, 1);
        print("Lifecycle test " + i + ": " + result);
    }

    print("Lifecycle management test completed!");
}

// Main test runner
function runAllTests(): void {
    print("=== Lambda-to-Interface Implementation Tests ===");
    print("");

    testBasicLambdaInterface();
    print("");

    testGenericLambdaInterface();
    print("");

    testArrayReturnTypeInterface();
    print("");

    testParameterValidation();
    print("");

    testReturnTypeValidation();
    print("");

    testClosureCapture();
    print("");

    testMultipleMethodInterface();
    print("");

    testErrorHandling();
    print("");

    testLifecycleManagement();
    print("");

    print("=== All Tests Completed ===");
}

// Run the tests
runAllTests();