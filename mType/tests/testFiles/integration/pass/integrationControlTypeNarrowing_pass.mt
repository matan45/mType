// Test: Type narrowing in control flow structures
// Tests type narrowing through conditionals, null checks, and type guards
import * from "../../../lib/primitives/Int.mt";

class Container<T> {
    private T value;
    private Bool hasValue;

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

    public Bool isEmpty() {
        return !this.hasValue;
    }
}

// Test 1: Type narrowing with null checks
print("Test 1: Type narrowing with null checks");
Container<Int> container = new Container<Int>();

if (container.isEmpty()) {
    print("Container is empty");
    container.setValue(new Int(42));
}

if (!container.isEmpty()) {
    Int value = container.getValue();
    print("Container value: " + value.getValue());
}

// Test 2: Type narrowing in conditionals with type inference
print("\nTest 2: Type narrowing in conditionals");

function processValue(Int val): String {
    if (val < 0) {
        return "Negative: " + val;
    } else {
        if (val == 0) {
            return "Zero";
        } else {
            return "Positive: " + val;
        }
    }
}

print(processValue(-5));
print(processValue(0));
print(processValue(10));

// Test 3: Type narrowing through boolean expressions
print("\nTest 3: Type narrowing with boolean logic");

class ValueHolder {
    private Int value;
    private Bool isValid;

    public constructor(Int v) {
        this.value = v;
        this.isValid = v >= 0 && v <= 100;
    }

    public Bool check() {
        return this.isValid;
    }

    public Int get() {
        return this.value;
    }
}

ValueHolder holder1 = new ValueHolder(50);
ValueHolder holder2 = new ValueHolder(-10);

if (holder1.check() && holder1.get() > 25) {
    print("Holder1 is valid and > 25: " + holder1.get());
}

if (!holder2.check() || holder2.get() < 0) {
    print("Holder2 is invalid or negative");
}

// Test 4: Type narrowing in while loops
print("\nTest 4: Type narrowing in loops");

Int counter = 0;
Container<String>[] containers = new Container<String>[3];

while (counter < 3) {
    containers[counter] = new Container<String>();
    if (counter % 2 == 0) {
        containers[counter].setValue("Even" + counter);
    }
    counter = counter + 1;
}

counter = 0;
while (counter < 3) {
    if (!containers[counter].isEmpty()) {
        String val = containers[counter].getValue();
        print("Container[" + counter + "]: " + val);
    } else {
        print("Container[" + counter + "]: empty");
    }
    counter = counter + 1;
}

// Test 5: Type narrowing with early returns
print("\nTest 5: Type narrowing with early returns");

class Range {
    private Int min;
    private Int max;

    public constructor(Int min, Int max) {
        this.min = min;
        this.max = max;
    }

    public Bool contains(Int value): Bool {
        if (value < this.min) {
            return false;
        }
        if (value > this.max) {
            return false;
        }
        return true;
    }

    public String describe(Int value): String {
        if (!this.contains(value)) {
            return "Out of range";
        }
        if (value == this.min) {
            return "At minimum";
        }
        if (value == this.max) {
            return "At maximum";
        }
        return "In range";
    }
}

Range range = new Range(10, 20);
print(range.describe(5));
print(range.describe(10));
print(range.describe(15));
print(range.describe(20));
print(range.describe(25));

// Test 6: Type narrowing with ternary operator
print("\nTest 6: Type narrowing with ternary");

function classify(Int num): String {
    return num > 0 ? "positive" : (num < 0 ? "negative" : "zero");
}

print("classify(10): " + classify(10));
print("classify(-5): " + classify(-5));
print("classify(0): " + classify(0));

print("\nType narrowing tests completed");
