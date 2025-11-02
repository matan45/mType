// Chaining based on predicates test
import * from "../../../lib/primitives/Int.mt";

interface Function<T, R> {
    function apply(T input) : R;
}

interface Predicate<T> {
    function test(T input) : bool;
}

class ConditionalProcessor<T> {
    T value;

    function init(T val) {
        this.value = val;
    }

    function when(Predicate<T> condition, Function<T, T> action) : ConditionalProcessor<T> {
        if (condition.test(this.value)) {
            this.value = action.apply(this.value);
        }
        return this;
    }

    function get() : T {
        return this.value;
    }
}

print("=== Conditional Chain Test ===");

// Simple conditional chain
ConditionalProcessor<Int> cp1 = new ConditionalProcessor<Int>(new Int(5));
Int result1 = cp1
    .when(x -> x.getValue() > 0, x -> new Int(x.getValue() * 2))
    .when(x -> x.getValue() > 5, x -> new Int(x.getValue() + 10))
    .when(x -> x.getValue() > 100, x -> new Int(x.getValue() - 50))
    .get();

print("Result 1: " + result1.getValue());  // 5 -> 10 -> 20 (stopped at x>100)

// Chain with all conditions true
ConditionalProcessor<Int> cp2 = new ConditionalProcessor<Int>(new Int(10));
Int result2 = cp2
    .when(x -> x.getValue() >= 10, x -> new Int(x.getValue() + 5))
    .when(x -> x.getValue() >= 15, x -> new Int(x.getValue() * 2))
    .when(x -> x.getValue() >= 20, x -> new Int(x.getValue() + 10))
    .get();

print("Result 2: " + result2.getValue());  // 10 -> 15 -> 30 -> 40

// Chain with no conditions true
ConditionalProcessor<Int> cp3 = new ConditionalProcessor<Int>(new Int(3));
Int result3 = cp3
    .when(x -> x.getValue() > 10, x -> new Int(x.getValue() * 100))
    .when(x -> x.getValue() < 0, x -> new Int(x.getValue() + 1000))
    .get();

print("Result 3: " + result3.getValue());  // 3 (unchanged)

// Complex conditional logic
ConditionalProcessor<Int> cp4 = new ConditionalProcessor<Int>(new Int(8));
Int result4 = cp4
    .when(x -> x.getValue() % 2 == 0, x -> new Int(x.getValue() / 2))
    .when(x -> x.getValue() < 5, x -> new Int(x.getValue() * 10))
    .when(x -> x.getValue() > 20, x -> new Int(x.getValue() - 10))
    .get();

print("Result 4: " + result4.getValue());  // 8 -> 4 -> 40 (stopped at x>20)

// Validation chain pattern
class Validator {
    String[] errors;
    int errorCount;

    function init() {
        this.errors = new String[10];
        this.errorCount = 0;
    }

    function validate(int value, Predicate<Int> check, String errorMsg) : Validator {
        if (!check.test(new Int(value))) {
            this.errors[this.errorCount] = errorMsg;
            this.errorCount = this.errorCount + 1;
        }
        return this;
    }

    function isValid() : bool {
        return this.errorCount == 0;
    }

    function printErrors() {
        for (int i = 0; i < this.errorCount; i = i + 1) {
            print("Error: " + this.errors[i]);
        }
    }
}

Validator v = new Validator();
v.validate(15, x -> x.getValue() > 0, "Must be positive")
 .validate(15, x -> x.getValue() < 100, "Must be less than 100")
 .validate(15, x -> x.getValue() % 5 == 0, "Must be divisible by 5");

print("Validation result: " + (v.isValid() ? "Valid" : "Invalid"));

Validator v2 = new Validator();
v2.validate(-5, x -> x.getValue() > 0, "Must be positive")
  .validate(-5, x -> x.getValue() < 100, "Must be less than 100");

print("Validation 2 result: " + (v2.isValid() ? "Valid" : "Invalid"));
v2.printErrors();

print("Conditional chain complete");
