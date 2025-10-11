// Test try-catch-finally with return statements in class methods
import * from "../../lib/exceptions/Exception.mt";
class Calculator {
    int value;

    constructor(int initialValue) {
        this.value = initialValue;
    }

    // Test 1: Return in try with finally
    public function divideWithFinally(int divisor): int {
        try {
            if (divisor == 0) {
                Exception e = new Exception("Division by zero");
                throw e;
            }
            int result = this.value / divisor;
            return result;
        } catch (Exception e) {
            print("Error: " + e.getMessage());
            return -1;
        } finally {
            print("Finally: cleanup after division");
        }
    }

    // Test 2: Nested try-catch-finally with return
    public function complexOperation(int x): int {
        try {
            print("Outer try: x = " + x);
            try {
                if (x < 0) {
                    Exception e = new Exception("Negative value");
                    throw e;
                }
                print("Inner try: processing " + x);
                int result = this.value + x;
                return result;
            } catch (Exception e) {
                print("Inner catch: " + e.getMessage());
                return -999;
            } finally {
                print("Inner finally");
            }
        } catch (Exception e) {
            print("Outer catch: " + e.getMessage());
            return -1;
        } finally {
            print("Outer finally");
        }
    }

    // Test 3: Multiple returns in try with finally
    public function multipleReturns(int mode): int {
        try {
            if (mode == 1) {
                print("Mode 1: returning value");
                return this.value;
            }
            if (mode == 2) {
                print("Mode 2: returning doubled value");
                return this.value * 2;
            }
            print("Default mode: returning tripled value");
            return this.value * 3;
        } finally {
            print("Finally: mode was " + mode);
        }
    }
}

// Test 1: Return in try with finally
print("=== Test 1: Return in try with finally ===");
Calculator calc1 = new Calculator(100);
int result1 = calc1.divideWithFinally(5);
print("Result 1: " + result1);

// Test with exception
int result2 = calc1.divideWithFinally(0);
print("Result 2: " + result2);

// Test 2: Nested try-catch-finally
print("\n=== Test 2: Nested try-catch-finally ===");
Calculator calc2 = new Calculator(50);
int result3 = calc2.complexOperation(10);
print("Result 3: " + result3);

// Test with exception
int result4 = calc2.complexOperation(-5);
print("Result 4: " + result4);

// Test 3: Multiple returns in try with finally
print("\n=== Test 3: Multiple returns with finally ===");
Calculator calc3 = new Calculator(10);
int result5 = calc3.multipleReturns(1);
print("Result 5: " + result5);

int result6 = calc3.multipleReturns(2);
print("Result 6: " + result6);

int result7 = calc3.multipleReturns(3);
print("Result 7: " + result7);

print("\nAll class try-catch-finally tests completed");
