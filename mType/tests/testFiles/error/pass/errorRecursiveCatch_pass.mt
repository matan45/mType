// Test: Catch block with recursive calls
// Expected: Should handle recursion within exception handlers correctly
import * from "../../lib/exceptions/Exception.mt";
import * from "../../lib/primitives/Int.mt";

class RecursiveException extends Exception {
    public Int depth;
    public Int maxDepth;

    public constructor(string msg, Int d, Int max) {
        super(msg);
        depth = d;
        maxDepth = max;
    }

    public function getDepth(): Int {
        return depth;
    }

    public function getMaxDepth(): Int {
        return maxDepth;
    }
}

// Recursive function that throws and catches exceptions
function recursiveThrowCatch(Int depth, Int maxDepth): Int {
    if (depth >= maxDepth) {
        throw new RecursiveException("Max depth reached", depth, maxDepth);
    }

    try {
        // Recursive call
        return recursiveThrowCatch(depth + 1, maxDepth);
    } catch (RecursiveException e) {
        // Catch and process, then re-throw or return
        if (depth == 0) {
            print("Bottom of recursion, caught exception");
            print("Final depth: " + e.getDepth());
            return e.getDepth();
        } else {
            // Continue unwinding
            throw e;
        }
    }
}

// Recursive exception handling with computation in catch block
function recursiveComputeInCatch(Int n): Int {
    if (n <= 1) {
        return new Int(1);
    }

    try {
        throw new RecursiveException("Computing factorial", n, n);
    } catch (RecursiveException e) {
        Int prev = recursiveComputeInCatch(n - 1);
        return n * prev;
    }
}

// Multiple recursive calls in catch block
function multiRecursiveInCatch(Int depth, Int maxDepth): void {
    if (depth >= maxDepth) {
        return;
    }

    try {
        throw new RecursiveException("Multi-recursive", depth, maxDepth);
    } catch (RecursiveException e) {
        if (depth % 10 == 0) {
            print("Catch at depth: " + depth);
        }

        // Make two recursive calls in catch block
        multiRecursiveInCatch(depth + 1, maxDepth);
        multiRecursiveInCatch(depth + 1, maxDepth);
    }
}

// Recursive exception with finally and recursive calls
function recursiveWithFinally(Int depth, Int maxDepth): Int {
    if (depth >= maxDepth) {
        throw new RecursiveException("Finally recursion", depth, maxDepth);
    }

    try {
        return recursiveWithFinally(depth + 1, maxDepth);
    } catch (RecursiveException e) {
        // Do computation in catch
        if (depth == 0) {
            print("Caught at bottom with finally");
            return e.getDepth();
        }
        throw e;
    } finally {
        // Finally block executes on each unwind
        if (depth % 20 == 0) {
            print("Finally at depth: " + depth);
        }
    }
}

// Tail-recursive exception handling
function tailRecursiveCatch(Int count, Int target, Int accumulated): Int {
    if (count >= target) {
        return accumulated;
    }

    try {
        throw new RecursiveException("Tail recursive", count, target);
    } catch (RecursiveException e) {
        // Tail-recursive call in catch
        return tailRecursiveCatch(count + 1, target, accumulated + count);
    }
}

// Mutually recursive exception handling
function mutualRecursiveA(Int depth, Int maxDepth): void;
function mutualRecursiveB(Int depth, Int maxDepth): void;

function mutualRecursiveA(Int depth, Int maxDepth): void {
    if (depth >= maxDepth) {
        return;
    }

    try {
        throw new RecursiveException("Mutual A", depth, maxDepth);
    } catch (RecursiveException e) {
        if (depth % 25 == 0) {
            print("Mutual A at depth: " + depth);
        }
        mutualRecursiveB(depth + 1, maxDepth);
    }
}

function mutualRecursiveB(Int depth, Int maxDepth): void {
    if (depth >= maxDepth) {
        return;
    }

    try {
        throw new RecursiveException("Mutual B", depth, maxDepth);
    } catch (RecursiveException e) {
        if (depth % 25 == 0) {
            print("Mutual B at depth: " + depth);
        }
        mutualRecursiveA(depth + 1, maxDepth);
    }
}

print("Starting recursive catch performance test");

print("Test 1: Basic recursive throw and catch (depth 50)");
Int result1 = recursiveThrowCatch(new Int(0), new Int(50));
print("Result: " + result1);

print("---");
print("Test 2: Recursive computation in catch (factorial of 10)");
Int result2 = recursiveComputeInCatch(new Int(10));
print("Result: " + result2);

print("---");
print("Test 3: Multiple recursive calls in catch (depth 30)");
multiRecursiveInCatch(new Int(0), new Int(30));
print("Multi-recursive completed");

print("---");
print("Test 4: Recursive with finally blocks (depth 60)");
Int result4 = recursiveWithFinally(new Int(0), new Int(60));
print("Result: " + result4);

print("---");
print("Test 5: Tail-recursive catch (100 iterations)");
Int result5 = tailRecursiveCatch(new Int(0), new Int(100), new Int(0));
print("Result: " + result5);

print("---");
print("Test 6: Mutually recursive exception handling (depth 50)");
mutualRecursiveA(new Int(0), new Int(50));
print("Mutual recursion completed");

print("---");
print("Recursive catch test completed successfully!");
