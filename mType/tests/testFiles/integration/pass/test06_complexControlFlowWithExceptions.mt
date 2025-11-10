// Integration Test 06: Complex Control Flow with Exceptions
// Tests: Switch-case + Nested loops + Try-catch-finally + Break/continue + Arrays
import * from "../../lib/exceptions/Exception.mt";
// Simple exception classes
class ValidationError extends Exception {
    constructor(string msg): super(msg) {
    }
}

class ProcessingError extends Exception {
    
    private int code;

    constructor(string msg, int c): super(msg) {
        this.code = c;
    }

    public function getCode(): int {
        return this.code;
    }
}

// Test switch with exception handling
function testSwitchWithExceptions(int value): string {
    try {
        switch (value) {
            case 1:
                print("Case 1: Normal processing");
                return "success-1";
            case 2:
                print("Case 2: Validation required");
                ValidationError err = new ValidationError("Validation failed for case 2");
                throw err;
            case 3:
                print("Case 3: Processing");
                return "success-3";
            case 4:
                print("Case 4: Critical error");
                ProcessingError procErr = new ProcessingError("Processing failed", 404);
                throw procErr;
            default:
                print("Default case");
                return "default";
        }
    } catch (ValidationError e) {
        print("Caught ValidationError: " + e.getMessage());
        return "error-validation";
    } catch (ProcessingError e) {
        print("Caught ProcessingError: " + e.getMessage() + " (code: " + e.getCode() + ")");
        return "error-processing";
    } finally {
        print("Finally: Cleanup for value " + value);
    }
}

// Test nested loops with break/continue
function testNestedLoopsWithControl(): void {
    print("--- Nested loops with break/continue ---");

    int[][] matrix = [[1, 2, 3], [4, 5, 6], [7, 8, 9]];

    for (int i = 0; i < matrix.length; i++) {
        print("Row " + i + ":");

        for (int j = 0; j < matrix[i].length; j++) {
            int value = matrix[i][j];

            // Skip even numbers
            if (value % 2 == 0) {
                continue;
            }

            print("  Odd value: " + value);

            // Break inner loop if value is 7
            if (value == 7) {
                print("  Breaking inner loop at 7");
                break;
            }
        }
    }
}

// Test loops with exception handling
function testLoopsWithExceptions(): void {
    print("--- Loops with exception handling ---");

    int[] numbers = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10];

    for (int i = 0; i < numbers.length; i++) {
        try {
            int num = numbers[i];

            // Skip multiples of 3
            if (num % 3 == 0) {
                print("Skipping " + num);
                continue;
            }

            // Throw exception for number 7
            if (num == 7) {
                ValidationError err = new ValidationError("Number 7 not allowed");
                throw err;
            }

            print("Processing: " + num);

        } catch (ValidationError e) {
            print("Caught error: " + e.getMessage());
            continue;
        } finally {
            // Finally always executes
        }
    }
}

// Test switch in loop with break
function testSwitchInLoop(): void {
    print("--- Switch in loop ---");

    for (int i = 1; i <= 10; i++) {
        string result = "";

        switch (i) {
            case 1:
            case 2:
            case 3:
                result = "Group A";
                break;
            case 4:
            case 5:
            case 6:
                result = "Group B";
                break;
            case 7:
            case 8:
                result = "Group C";
                break;
            default:
                result = "Group D";
                break;
        }

        print(i + " -> " + result);

        // Break out of loop at 7
        if (i == 7) {
            print("Breaking at 7");
            break;
        }
    }
}

// Test do-while with exception handling
function testDoWhileWithExceptions(): void {
    print("--- Do-while with exceptions ---");

    int counter = 0;

    do {
        try {
            counter = counter + 1;

            if (counter == 3) {
                ValidationError err = new ValidationError("Counter reached 3");
                throw err;
            }

            print("Counter: " + counter);

        } catch (ValidationError e) {
            print("Error at counter " + counter + ": " + e.getMessage());
            break;  // Exit do-while
        }

    } while (counter < 10);

    print("Final counter: " + counter);
}

// Main test execution
print("=== Test 06: Complex Control Flow with Exceptions ===");

// Test 1: Switch with exceptions
print("--- Testing switch with exceptions ---");
string r1 = testSwitchWithExceptions(1);
print("Result: " + r1);

string r2 = testSwitchWithExceptions(2);
print("Result: " + r2);

string r3 = testSwitchWithExceptions(4);
print("Result: " + r3);

string r4 = testSwitchWithExceptions(99);
print("Result: " + r4);

// Test 2: Nested loops with break/continue
testNestedLoopsWithControl();

// Test 3: Loops with exception handling
testLoopsWithExceptions();

// Test 4: Switch in loop
testSwitchInLoop();

// Test 5: Do-while with exceptions
testDoWhileWithExceptions();

print("=== Test 06 Complete ===");
