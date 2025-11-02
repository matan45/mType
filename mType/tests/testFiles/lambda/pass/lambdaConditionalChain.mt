// Chaining based on predicates test
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
ConditionalProcessor<int> cp1 = new ConditionalProcessor<int>(5);
int result1 = cp1
    .when(x -> x > 0, x -> x * 2)
    .when(x -> x > 5, x -> x + 10)
    .when(x -> x > 100, x -> x - 50)
    .get();

print("Result 1: " + result1);  // 5 -> 10 -> 20 (stopped at x>100)

// Chain with all conditions true
ConditionalProcessor<int> cp2 = new ConditionalProcessor<int>(10);
int result2 = cp2
    .when(x -> x >= 10, x -> x + 5)
    .when(x -> x >= 15, x -> x * 2)
    .when(x -> x >= 20, x -> x + 10)
    .get();

print("Result 2: " + result2);  // 10 -> 15 -> 30 -> 40

// Chain with no conditions true
ConditionalProcessor<int> cp3 = new ConditionalProcessor<int>(3);
int result3 = cp3
    .when(x -> x > 10, x -> x * 100)
    .when(x -> x < 0, x -> x + 1000)
    .get();

print("Result 3: " + result3);  // 3 (unchanged)

// Complex conditional logic
ConditionalProcessor<int> cp4 = new ConditionalProcessor<int>(8);
int result4 = cp4
    .when(x -> x % 2 == 0, x -> x / 2)
    .when(x -> x < 5, x -> x * 10)
    .when(x -> x > 20, x -> x - 10)
    .get();

print("Result 4: " + result4);  // 8 -> 4 -> 40 (stopped at x>20)

// Validation chain pattern
class Validator {
    String[] errors;
    int errorCount;

    function init() {
        this.errors = new String[10];
        this.errorCount = 0;
    }

    function validate(int value, Predicate<int> check, String errorMsg) : Validator {
        if (!check.test(value)) {
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
v.validate(15, x -> x > 0, "Must be positive")
 .validate(15, x -> x < 100, "Must be less than 100")
 .validate(15, x -> x % 5 == 0, "Must be divisible by 5");

print("Validation result: " + (v.isValid() ? "Valid" : "Invalid"));

Validator v2 = new Validator();
v2.validate(-5, x -> x > 0, "Must be positive")
  .validate(-5, x -> x < 100, "Must be less than 100");

print("Validation 2 result: " + (v2.isValid() ? "Valid" : "Invalid"));
v2.printErrors();

print("Conditional chain complete");
