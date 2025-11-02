// Test: Throwing and catching many exceptions (1000+)
// Expected: Should handle large volume of exceptions efficiently
import * from "../../lib/exceptions/Exception.mt";
import * from "../../lib/primitives/Int.mt";

class CountException extends Exception {
    public Int count;

    public constructor(string msg, Int cnt) {
        super(msg);
        count = cnt;
    }

    public function getCount(): Int {
        return count;
    }
}

// Throw and catch many exceptions in sequence
function testManySequentialExceptions(Int iterations): void {
    Int caught = new Int(0);

    Int i = new Int(0);
    while (i < iterations) {
        try {
            throw new CountException("Exception number", i);
        } catch (CountException e) {
            caught = caught + 1;
        }
        i = i + 1;
    }

    print("Sequential exceptions caught: " + caught);
}

// Throw and catch exceptions in nested loops
function testNestedLoopExceptions(Int outer, Int inner): void {
    Int totalCaught = new Int(0);

    Int i = new Int(0);
    while (i < outer) {
        Int j = new Int(0);
        while (j < inner) {
            try {
                throw new CountException("Nested exception", i * inner + j);
            } catch (CountException e) {
                totalCaught = totalCaught + 1;
            }
            j = j + 1;
        }
        i = i + 1;
    }

    print("Nested loop exceptions caught: " + totalCaught);
}

// Multiple catch clauses with many exceptions
class TypeAException extends Exception {
    constructor(string msg) { super(msg); }
}

class TypeBException extends Exception {
    constructor(string msg) { super(msg); }
}

class TypeCException extends Exception {
    constructor(string msg) { super(msg); }
}

function testMultipleCatchTypes(Int iterations): void {
    Int caughtA = new Int(0);
    Int caughtB = new Int(0);
    Int caughtC = new Int(0);

    Int i = new Int(0);
    while (i < iterations) {
        try {
            Int mod = i % 3;
            if (mod == 0) {
                throw new TypeAException("Type A");
            } else if (mod == 1) {
                throw new TypeBException("Type B");
            } else {
                throw new TypeCException("Type C");
            }
        } catch (TypeAException e) {
            caughtA = caughtA + 1;
        } catch (TypeBException e) {
            caughtB = caughtB + 1;
        } catch (TypeCException e) {
            caughtC = caughtC + 1;
        }
        i = i + 1;
    }

    print("Type A caught: " + caughtA);
    print("Type B caught: " + caughtB);
    print("Type C caught: " + caughtC);
}

// Exception with finally blocks
function testManyExceptionsWithFinally(Int iterations): void {
    Int caught = new Int(0);
    Int finallyCalled = new Int(0);

    Int i = new Int(0);
    while (i < iterations) {
        try {
            throw new CountException("Finally test", i);
        } catch (CountException e) {
            caught = caught + 1;
        } finally {
            finallyCalled = finallyCalled + 1;
        }
        i = i + 1;
    }

    print("Caught with finally: " + caught);
    print("Finally blocks executed: " + finallyCalled);
}

print("Starting many exceptions performance test");

print("Test 1: 1000 sequential exceptions");
testManySequentialExceptions(new Int(1000));

print("---");
print("Test 2: 50x20 nested loop exceptions (1000 total)");
testNestedLoopExceptions(new Int(50), new Int(20));

print("---");
print("Test 3: 1000 exceptions with multiple catch types");
testMultipleCatchTypes(new Int(1000));

print("---");
print("Test 4: 500 exceptions with finally blocks");
testManyExceptionsWithFinally(new Int(500));

print("---");
print("Many exceptions test completed successfully!");
