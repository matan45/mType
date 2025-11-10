// Integration Test 04: Lambda Closures with Auto-Boxing
// Tests: Lambdas + Auto-boxing + Collections + For loops + Scoping

import * from "../../lib/collections/List.mt";
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";
import * from "../../lib/primitives/Bool.mt";

// Functional interfaces for lambdas
interface Function {
    function apply(int x): int;
}

interface Predicate {
    function test(int x): bool;
}

interface Transformer {
    function transform(int x): String;
}

// Lambda implementation with closure
class ClosureFunction implements Function {
    private int captured;

    constructor(int cap) {
        this.captured = cap;
    }

    public function apply(int x): int {
        return x + this.captured;
    }
}

class PredicateImpl implements Predicate {
    private int threshold;

    constructor(int thresh) {
        this.threshold = thresh;
    }

    public function test(int x): bool {
        return x > this.threshold;
    }
}

class StringTransformer implements Transformer {
    private string prefix;

    constructor(string pre) {
        this.prefix = pre;
    }

    public function transform(int x): String {
        return new String(this.prefix + x);
    }
}

// Container using lambdas
class FunctionalContainer {
    private List<Int> numbers;

    constructor() {
        this.numbers = new List<Int>();
    }

    public function addNumber(Int num): void {
        this.numbers.add(num);
    }

    public function mapWithFunction(Function func): List<Int> {
        List<Int> result = new List<Int>();
        for (int i = 0; i < this.numbers.size(); i = i + 1) {
            Int current = this.numbers.get(i);
            int mapped = func.apply(current.getValue());
            result.add(new Int(mapped));  // Auto-boxing: int -> Int
        }
        return result;
    }

    public function filter(Predicate pred): List<Int> {
        List<Int> result = new List<Int>();
        for (int i = 0; i < this.numbers.size(); i = i + 1) {
            Int current = this.numbers.get(i);
            bool passes = pred.test(current.getValue());
            if (passes) {
                result.add(current);
            }
        }
        return result;
    }

    public function transformToStrings(Transformer trans): List<String> {
        List<String> result = new List<String>();
        for (int i = 0; i < this.numbers.size(); i = i + 1) {
            Int current = this.numbers.get(i);
            String transformed = trans.transform(current.getValue());
            result.add(transformed);
        }
        return result;
    }
}

// Test auto-boxing with operators
function testAutoBoxing(): void {
    print("--- Auto-boxing tests ---");

    // int -> Int auto-boxing
    Int a = 10;  // Auto-boxes 10 to new Int(10)
    Int b = 20;  // Auto-boxes 20 to new Int(20)

    // Arithmetic operators with auto-boxing
    Int sum = a + b;  // Calls a.add(b)
    print("Sum: " + sum.getValue());

    Int diff = a - b;  // Calls a.subtract(b)
    print("Difference: " + diff.getValue());

    Int prod = a * b;  // Calls a.multiply(b)
    print("Product: " + prod.getValue());

    // Comparison operators with auto-boxing
    Bool isLess = a < b;  // Calls a.lessThan(b)
    print("10 < 20: " + isLess.getValue());

    Bool isEqual = a == a;  // Calls a.equals(a)
    print("10 == 10: " + isEqual.getValue());

    // String auto-boxing
    String s = "Hello";  // Auto-boxes to new String("Hello")
    print("String value: " + s.getValue());
}

// Test scoping with nested blocks
function testScoping(): void {
    print("--- Scoping tests ---");

    int outerVar = 100;
    print("Outer: " + outerVar);

    {
        int blockVar = 200;
        print("Block: " + blockVar);

        {
            int nestedVar = 300;
            print("Nested: " + nestedVar);
            print("Can access outer: " + outerVar);
        }
    }

    // blockVar and nestedVar are out of scope here
    for (int i = 1; i <= 3; i++) {
        int loopVar = i * 10;
        print("Loop var: " + loopVar);
    }
}

// Main test execution
print("=== Test 04: Lambda Closures with Auto-Boxing ===");

// Test 1: Auto-boxing
testAutoBoxing();

// Test 2: Lambda with closure capturing
print("--- Lambda closure tests ---");
FunctionalContainer container = new FunctionalContainer();

// Add numbers with auto-boxing
container.addNumber(5);   // Auto-boxes to new Int(5)
container.addNumber(10);
container.addNumber(15);
container.addNumber(20);
container.addNumber(25);

// Map with closure (add 100 to each)
Function adder = new ClosureFunction(100);
List<Int> mapped = container.mapWithFunction(adder);

print("Mapped results (add 100):");
for (int i = 0; i < mapped.size(); i++) {
    Int val = mapped.get(i);
    print(val.getValue());
}

// Filter with predicate (greater than 12)
Predicate greaterThan12 = new PredicateImpl(12);
List<Int> filtered = container.filter(greaterThan12);

print("Filtered results (> 12):");
for (int i = 0; i < filtered.size(); i++) {
    Int val = filtered.get(i);
    print(val.getValue());
}

// Transform to strings
Transformer transformer = new StringTransformer("Number: ");
List<String> strings = container.transformToStrings(transformer);

print("Transformed to strings:");
for (int i = 0; i < strings.size(); i++) {
    String str = strings.get(i);
    print(str.getValue());
}

// Test 3: Nested closures
print("--- Nested closure test ---");
int baseValue = 50;
Function nested = new ClosureFunction(baseValue);

for (int i = 1; i <= 3; i++) {
    int result = nested.apply(i);
    print("Apply to " + i + ": " + result);
}

// Test 4: Scoping
testScoping();

print("=== Test 04 Complete ===");
