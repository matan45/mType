// Test: Methods with multiple early return statements
// Expected: Pass - demonstrates early return patterns
import * from "../../lib/exceptions/Exception.mt";

class Validator {
    // Multiple early returns for validation
    public function validate(int age, string name, string email): string {
        print("Validating: age=" + age + ", name=" + name + ", email=" + email);

        if (age < 0) {
            print("Early return: invalid age");
            return "Error: Age cannot be negative";
        }

        if (age < 18) {
            print("Early return: too young");
            return "Error: Must be 18 or older";
        }

        if (name == "") {
            print("Early return: empty name");
            return "Error: Name is required";
        }

        if (email == "") {
            print("Early return: empty email");
            return "Error: Email is required";
        }

        print("All validations passed");
        return "Valid";
    }

    // Early return in loop
    public function findFirst(int target): int {
        print("Finding first occurrence of " + target);
        int i = 0;
        while (i < 10) {
            print("Checking position " + i);
            if (i == target) {
                print("Found at position " + i);
                return i;
            }
            i = i + 1;
        }
        print("Not found");
        return -1;
    }

    // Early return with try-catch
    public function processWithException(bool shouldThrow): string {
        print("Processing with shouldThrow=" + shouldThrow);
        try {
            if (shouldThrow) {
                print("Throwing exception");
                throw new Exception("TestException");
            }
            print("No exception, returning success");
            return "Success";
        } catch (Exception e) {
            print("Caught exception: " + e.getMessage());
            return "Error: " + e.getMessage();
        }
    }
}

Validator v = new Validator();

print("Test 1: Invalid age");
print("Result: " + v.validate(-5, "John", "john@test.com"));

print("\nTest 2: Too young");
print("Result: " + v.validate(15, "Jane", "jane@test.com"));

print("\nTest 3: Empty name");
print("Result: " + v.validate(25, "", "test@test.com"));

print("\nTest 4: Valid input");
print("Result: " + v.validate(30, "Bob", "bob@test.com"));

print("\nTest 5: Find first");
int pos = v.findFirst(5);
print("Position: " + pos);

print("\nTest 6: Exception handling");
print("Result: " + v.processWithException(true));
print("Result: " + v.processWithException(false));
