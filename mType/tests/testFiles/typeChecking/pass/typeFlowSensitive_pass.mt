// Test: Flow-sensitive typing with conditional branches
// Expected: Pass - type information flows through control structures
import * from "../../../lib/primitives/Int.mt";

class Container<T> {
    private T value;
    private bool hasValue;

    public constructor() {
        this.hasValue = false;
    }

    public void setValue(T v) {
        this.value = v;
        this.hasValue = true;
    }

    public T getValue() {
        return this.value;
    }

    public bool isEmpty() {
        return !this.hasValue;
    }
}

// Test 1: Flow-sensitive type through conditionals
print("Test 1: Flow-sensitive conditionals");
Container<Int> intContainer = new Container<Int>();

if (intContainer.isEmpty()) {
    print("Container is empty");
    intContainer.setValue(new Int(42));
}

if (!intContainer.isEmpty()) {
    print("Container has value: " + intContainer.getValue().getValue());
}

// Test 2: Flow through loops
print("\nTest 2: Flow-sensitive in loops");
Container<string>[] containers = new Container<string>[3];
int i = 0;
while (i < 3) {
    containers[i] = new Container<string>();
    i = i + 1;
}

i = 0;
while (i < 3) {
    if (containers[i].isEmpty()) {
        containers[i].setValue("Value" + i);
    }
    i = i + 1;
}

i = 0;
while (i < 3) {
    if (!containers[i].isEmpty()) {
        print("Container " + i + ": " + containers[i].getValue());
    }
    i = i + 1;
}

// Test 3: Flow through multiple branches
class State {
    private int status;
    private string message;

    public constructor(int status) {
        this.status = status;
        if (status == 0) {
            this.message = "Success";
        } else {
            if (status == 1) {
                this.message = "Warning";
            } else {
                this.message = "Error";
            }
        }
    }

    public string getMessage() {
        return this.message;
    }

    public int getStatus() {
        return this.status;
    }
}

print("\nTest 3: Flow through branches");
State state1 = new State(0);
print("Status 0: " + state1.getMessage());

State state2 = new State(1);
print("Status 1: " + state2.getMessage());

State state3 = new State(2);
print("Status 2: " + state3.getMessage());

// Test 4: Flow-sensitive type narrowing through assignments
class NumberWrapper {
    private int value;

    public constructor(int v) {
        this.value = v;
    }

    public int getValue() {
        return this.value;
    }

    public void setValue(int v) {
        this.value = v;
    }
}

print("\nTest 4: Flow through reassignments");
NumberWrapper wrapper = new NumberWrapper(10);
print("Initial value: " + wrapper.getValue());

if (wrapper.getValue() < 20) {
    wrapper.setValue(wrapper.getValue() + 10);
    print("After increment: " + wrapper.getValue());
}

if (wrapper.getValue() >= 20) {
    print("Value is now >= 20: " + wrapper.getValue());
}

// Test 5: Flow through nested conditions
class Validator {
    public bool validate(int value): bool {
        bool result = false;
        if (value > 0) {
            if (value < 100) {
                result = true;
            }
        }
        return result;
    }

    public string validateAndReport(int value): string {
        if (this.validate(value)) {
            return "Valid: " + value;
        }
        return "Invalid: " + value;
    }
}

print("\nTest 5: Flow through nested validation");
Validator validator = new Validator();
print(validator.validateAndReport(50));
print(validator.validateAndReport(-5));
print(validator.validateAndReport(150));

// Test 6: Flow-sensitive with early returns
class Calculator {
    public int divide(int a, int b): int {
        if (b == 0) {
            print("Cannot divide by zero, returning 0");
            return 0;
        }
        return a / b;
    }

    public int safeDivide(int a, int b): int {
        int result = this.divide(a, b);
        if (result == 0) {
            if (b != 0) {
                print("Result is zero");
            }
        } else {
            print("Result is non-zero: " + result);
        }
        return result;
    }
}

print("\nTest 6: Flow-sensitive with division");
Calculator calc = new Calculator();
calc.safeDivide(10, 2);
calc.safeDivide(10, 0);
calc.safeDivide(0, 5);

print("\nFlow-sensitive typing tests completed");
