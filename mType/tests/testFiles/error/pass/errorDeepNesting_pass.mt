// Test: Very deep exception nesting (100+ levels)
// Expected: Should handle deep nesting without stack overflow
import * from "../../lib/exceptions/Exception.mt";
import * from "../../lib/primitives/Int.mt";

class DeepException extends Exception {
    public Int level;

    public constructor(string msg, Int lvl) {
        super(msg);
        level = lvl;
    }

    public function getLevel(): Int {
        return level;
    }
}

// Recursively nest try-catch blocks to test deep exception handling
function deepNest(Int depth, Int maxDepth): void {
    if (depth >= maxDepth) {
        throw new DeepException("Reached max depth", depth);
    }

    try {
        deepNest(depth + 1, maxDepth);
    } catch (DeepException e) {
        // Re-throw to propagate up the stack
        if (depth > 0) {
            throw e;
        } else {
            // Bottom of stack - verify we caught it
            print("Caught exception at bottom: " + e.getMessage());
            print("Max depth reached: " + e.getLevel());
        }
    }
}

// Test with multiple nested levels
function testNestedExceptions(): void {
    Int depth = new Int(0);
    Int level1 = new Int(10);
    Int level2 = new Int(50);
    Int level3 = new Int(100);

    print("Testing deep nesting with 10 levels:");
    try {
        deepNest(depth, level1);
    } catch (DeepException e) {
        print("Caught at level 10: " + e.getLevel());
    }

    print("Testing deep nesting with 50 levels:");
    try {
        deepNest(depth, level2);
    } catch (DeepException e) {
        print("Caught at level 50: " + e.getLevel());
    }

    print("Testing deep nesting with 100 levels:");
    try {
        deepNest(depth, level3);
    } catch (DeepException e) {
        print("Caught at level 100: " + e.getLevel());
    }
}

// Test deeply nested try-finally blocks
function deepFinallyNest(Int depth, Int maxDepth): void {
    if (depth >= maxDepth) {
        throw new DeepException("Finally nesting complete", depth);
    }

    try {
        deepFinallyNest(depth + 1, maxDepth);
    } finally {
        // Each finally should execute on unwind
        if (depth % 10 == 0) {
            print("Finally at depth: " + depth);
        }
    }
}

function testDeepFinally(): void {
    print("Testing deep finally nesting:");
    try {
        Int depth = new Int(0);
        Int maxDepth = new Int(100);
        deepFinallyNest(depth, maxDepth);
    } catch (DeepException e) {
        print("Caught after finally unwinding: " + e.getLevel());
    }
}

print("Starting deep exception nesting performance test");
testNestedExceptions();
print("---");
testDeepFinally();
print("Deep nesting test completed successfully!");
