// Integration Test 11: Scoping Edge Cases with Lambdas
// Tests: Variable shadowing prevention + Lambda capture + Block scoping

import * from "../../../lib/collections/List.mt";
import * from "../../../lib/primitives/Int.mt";

// Functional interfaces
interface Function {
    function apply(int x): int;
}

interface Transformer {
    function transform(int x, int y): int;
}

// Test class-level scoping
class ScopingTest {
    private int instanceVar;
    private static int staticVar = 100;

    constructor(int val) {
        this.instanceVar = val;
    }

    // Method accessing instance variable
    public function createAdder(): Function {
        // Lambda captures instance variable
        class AdderFunction implements Function {
            private int captured;

            constructor(int cap) {
                this.captured = cap;
            }

            public function apply(int x): int {
                return x + this.captured;
            }
        }

        return new AdderFunction(this.instanceVar);
    }

    // Method with local variable
    public function createMultiplier(int factor): Function {
        // Lambda captures both instance and local variable
        class MultiplierFunction implements Function {
            private int capturedInstance;
            private int capturedLocal;

            constructor(int capInst, int capLoc) {
                this.capturedInstance = capInst;
                this.capturedLocal = capLoc;
            }

            public function apply(int x): int {
                return (x + this.capturedInstance) * this.capturedLocal;
            }
        }

        return new MultiplierFunction(this.instanceVar, factor);
    }

    // Static method accessing static variable
    public static function createStaticFunction(): Function {
        class StaticFunction implements Function {
            private int capturedStatic;

            constructor(int capStat) {
                this.capturedStatic = capStat;
            }

            public function apply(int x): int {
                return x + this.capturedStatic;
            }
        }

        return new StaticFunction(ScopingTest::staticVar);
    }

    public function getInstanceVar(): int {
        return this.instanceVar;
    }
}

// Test nested block scoping
function testNestedBlocks(): void {
    print("--- Nested block scoping ---");

    int outerVar = 10;
    print("Outer: " + outerVar);

    {
        int middleVar = 20;
        print("Middle: " + middleVar);
        print("Can access outer: " + outerVar);

        {
            int innerVar = 30;
            print("Inner: " + innerVar);
            print("Can access middle: " + middleVar);
            print("Can access outer: " + outerVar);
        }

        print("After inner block, can still access middle: " + middleVar);
    }

    print("After all blocks, can still access outer: " + outerVar);
}

// Test loop variable scoping
function testLoopScoping(): void {
    print("--- Loop variable scoping ---");

    for (int i = 0; i < 3; i = i + 1) {
        int loopLocal = i * 10;
        print("Loop " + i + ": loopLocal = " + loopLocal);
    }

    print("After loop, i is out of scope");

    // New loop with same variable name (allowed)
    for (int i = 10; i < 13; i = i + 1) {
        print("Second loop i: " + i);
    }
}

// Test lambda capture of loop variables
function testLambdaCaptureInLoop(): void {
    print("--- Lambda capture in loop ---");

    List<Function> functions = new List<Function>();

    for (int i = 0; i < 3; i = i + 1) {
        int captureValue = i;  // Capture in local variable

        class CaptureFunction implements Function {
            private int captured;

            constructor(int cap) {
                this.captured = cap;
            }

            public function apply(int x): int {
                return x + this.captured;
            }
        }

        functions.add(new CaptureFunction(captureValue));
    }

    print("Applying captured functions:");
    for (int i = 0; i < functions.size(); i = i + 1) {
        Function func = functions.get(i);
        int result = func.apply(100);
        print("Function " + i + " result: " + result);
    }
}

// Test closure with mutable state
function testMutableCapture(): void {
    print("--- Mutable capture ---");

    class Counter {
        private int count;

        constructor() {
            this.count = 0;
        }

        public function increment(): void {
            this.count = this.count + 1;
        }

        public function getCount(): int {
            return this.count;
        }
    }

    Counter counter = new Counter();

    class IncrementFunction implements Function {
        private Counter capturedCounter;

        constructor(Counter c) {
            this.capturedCounter = c;
        }

        public function apply(int x): int {
            this.capturedCounter.increment();
            return this.capturedCounter.getCount();
        }
    }

    Function incrementer = new IncrementFunction(counter);

    print("Count: " + counter.getCount());
    incrementer.apply(0);
    print("After 1st call: " + counter.getCount());
    incrementer.apply(0);
    print("After 2nd call: " + counter.getCount());
    incrementer.apply(0);
    print("After 3rd call: " + counter.getCount());
}

// Test parameter scoping in methods
function testParameterScoping(int param): void {
    print("--- Parameter scoping ---");
    print("Parameter value: " + param);

    int localVar = param * 2;
    print("Local derived from param: " + localVar);

    {
        int blockVar = param + 10;
        print("Block var: " + blockVar);
    }
}

// Main test execution
print("=== Test 11: Scoping Edge Cases with Lambdas ===");

// Test 1: Class-level scoping
print("--- Class-level scoping ---");
ScopingTest obj = new ScopingTest(50);

Function adder = obj.createAdder();
print("Adder result: " + adder.apply(25));

Function multiplier = obj.createMultiplier(3);
print("Multiplier result: " + multiplier.apply(10));

Function staticFunc = ScopingTest::createStaticFunction();
print("Static function result: " + staticFunc.apply(5));

// Test 2: Nested block scoping
testNestedBlocks();

// Test 3: Loop variable scoping
testLoopScoping();

// Test 4: Lambda capture in loop
testLambdaCaptureInLoop();

// Test 5: Mutable capture
testMutableCapture();

// Test 6: Parameter scoping
testParameterScoping(42);

// Test 7: Multiple instances with different captured values
print("--- Multiple instances ---");
ScopingTest obj1 = new ScopingTest(10);
ScopingTest obj2 = new ScopingTest(20);

Function f1 = obj1.createAdder();
Function f2 = obj2.createAdder();

print("f1 result: " + f1.apply(5));
print("f2 result: " + f2.apply(5));

print("=== Test 11 Complete ===");
