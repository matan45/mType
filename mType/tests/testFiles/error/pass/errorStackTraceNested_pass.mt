// Test: Deeply nested stack trace
// Expected: Stack traces should accurately reflect deep call chains
import * from "../../lib/exceptions/Exception.mt";
import * from "../../lib/exceptions/RuntimeException.mt";
import * from "../../lib/primitives/String.mt";
import * from "../../lib/primitives/Int.mt";

// Test 1: Deep function call stack (10 levels)
function level10(): void {
    print("Level 10: Throwing exception");
    RuntimeException e = new RuntimeException("Error at level 10");
    throw e;
}

function level9(): void {
    print("Level 9");
    level10();
}

function level8(): void {
    print("Level 8");
    level9();
}

function level7(): void {
    print("Level 7");
    level8();
}

function level6(): void {
    print("Level 6");
    level7();
}

function level5(): void {
    print("Level 5");
    level6();
}

function level4(): void {
    print("Level 4");
    level5();
}

function level3(): void {
    print("Level 3");
    level4();
}

function level2(): void {
    print("Level 2");
    level3();
}

function level1(): void {
    print("Level 1");
    level2();
}

function testDeepStack(): void {
    print("=== Test 1: Deep function stack (10 levels) ===");
    try {
        level1();
    } catch (RuntimeException e) {
        print("Caught: " + e.getMessage());
        string trace = e.getStackTrace();
        if (trace != "") {
            print("Deep stack trace captured: true");
        }
        print("Deep stack exception handled");
    }
}

// Test 2: Deep nested classes with methods
class Layer1 {
    public Layer2 next;

    public constructor() {
        next = new Layer2();
    }

    public function call(): void {
        print("Layer 1 calling Layer 2");
        next.call();
    }
}

class Layer2 {
    public Layer3 next;

    public constructor() {
        next = new Layer3();
    }

    public function call(): void {
        print("Layer 2 calling Layer 3");
        next.call();
    }
}

class Layer3 {
    public Layer4 next;

    public constructor() {
        next = new Layer4();
    }

    public function call(): void {
        print("Layer 3 calling Layer 4");
        next.call();
    }
}

class Layer4 {
    public Layer5 next;

    public constructor() {
        next = new Layer5();
    }

    public function call(): void {
        print("Layer 4 calling Layer 5");
        next.call();
    }
}

class Layer5 {
    public function call(): void {
        print("Layer 5: Final layer throwing");
        RuntimeException e = new RuntimeException("Error in Layer 5");
        throw e;
    }
}

function testNestedClasses(): void {
    print("=== Test 2: Nested class method calls ===");
    Layer1 layer = new Layer1();

    try {
        layer.call();
    } catch (RuntimeException e) {
        print("Caught: " + e.getMessage());
        string trace = e.getStackTrace();
        if (trace != "") {
            print("Nested class stack trace: true");
        }
        print("Nested class exception handled");
    }
}

// Test 3: Recursive function with depth limit
function recursiveFunction(int depth): void {
    print("Recursion depth: " + depth);

    if (depth >= 8) {
        print("Max depth reached, throwing");
        RuntimeException e = new RuntimeException("Recursion limit at depth " + depth);
        throw e;
    }

    recursiveFunction(depth + 1);
}

function testRecursiveStack(): void {
    print("=== Test 3: Recursive function stack ===");
    try {
        recursiveFunction(0);
    } catch (RuntimeException e) {
        print("Caught: " + e.getMessage());
        string trace = e.getStackTrace();
        if (trace != "") {
            print("Recursive stack trace: true");
        }
        print("Recursive exception handled");
    }
}

// Test 4: Mixed function and method calls
class Processor {
    public function process(int value): int {
        print("Processor.process: " + value);
        return helperFunction(value);
    }
}

function helperFunction(int val): int {
    print("helperFunction: " + val);
    Validator v = new Validator();
    return v.validate(val);
}

class Validator {
    public function validate(int input): int {
        print("Validator.validate: " + input);
        return checkValue(input);
    }
}

function checkValue(int n): int {
    print("checkValue: " + n);
    if (n > 50) {
        RuntimeException e = new RuntimeException("Value exceeds maximum");
        throw e;
    }
    return n;
}

function testMixedCalls(): void {
    print("=== Test 4: Mixed function and method calls ===");
    Processor proc = new Processor();

    try {
        int result = proc.process(75);
        print("Result: " + result);
    } catch (RuntimeException e) {
        print("Caught: " + e.getMessage());
        string trace = e.getStackTrace();
        if (trace != "") {
            print("Mixed call stack trace: true");
        }
        print("Mixed call exception handled");
    }
}

// Test 5: Deep try-catch nesting
function deepTryCatch5(): void {
    try {
        print("Try level 5");
        RuntimeException e = new RuntimeException("Error at try level 5");
        throw e;
    } catch (RuntimeException e) {
        print("Caught at level 5: " + e.getMessage());
        print("Re-throwing from level 5");
        throw e;
    }
}

function deepTryCatch4(): void {
    try {
        print("Try level 4");
        deepTryCatch5();
    } catch (RuntimeException e) {
        print("Caught at level 4");
        print("Re-throwing from level 4");
        throw e;
    }
}

function deepTryCatch3(): void {
    try {
        print("Try level 3");
        deepTryCatch4();
    } catch (RuntimeException e) {
        print("Caught at level 3");
        print("Re-throwing from level 3");
        throw e;
    }
}

function deepTryCatch2(): void {
    try {
        print("Try level 2");
        deepTryCatch3();
    } catch (RuntimeException e) {
        print("Caught at level 2");
        print("Re-throwing from level 2");
        throw e;
    }
}

function deepTryCatch1(): void {
    try {
        print("Try level 1");
        deepTryCatch2();
    } catch (RuntimeException e) {
        print("Caught at level 1");
        print("Re-throwing from level 1");
        throw e;
    }
}

function testDeepTryCatch(): void {
    print("=== Test 5: Deep try-catch nesting ===");
    try {
        deepTryCatch1();
    } catch (RuntimeException e) {
        print("Final catch: " + e.getMessage());
        string trace = e.getStackTrace();
        if (trace != "") {
            print("Deep try-catch stack trace: true");
        }
        print("Deep try-catch exception handled");
    }
}

// Test 6: Chain of constructor calls
class ConstructorChain1 {
    public ConstructorChain2 next;

    public constructor() {
        print("ConstructorChain1");
        next = new ConstructorChain2();
    }
}

class ConstructorChain2 {
    public ConstructorChain3 next;

    public constructor() {
        print("ConstructorChain2");
        next = new ConstructorChain3();
    }
}

class ConstructorChain3 {
    public ConstructorChain4 next;

    public constructor() {
        print("ConstructorChain3");
        next = new ConstructorChain4();
    }
}

class ConstructorChain4 {
    public constructor() {
        print("ConstructorChain4: Throwing from constructor");
        RuntimeException e = new RuntimeException("Constructor chain error");
        throw e;
    }
}

function testConstructorChain(): void {
    print("=== Test 6: Constructor call chain ===");
    try {
        ConstructorChain1 obj = new ConstructorChain1();
        print("Object created");
    } catch (RuntimeException e) {
        print("Caught: " + e.getMessage());
        string trace = e.getStackTrace();
        if (trace != "") {
            print("Constructor chain stack trace: true");
        }
        print("Constructor chain exception handled");
    }
}

// Run all tests
testDeepStack();
print("");
testNestedClasses();
print("");
testRecursiveStack();
print("");
testMixedCalls();
print("");
testDeepTryCatch();
print("");
testConstructorChain();
print("");
print("Nested stack trace test completed");
