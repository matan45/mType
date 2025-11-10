// Test: Throwing and catching many exceptions (1000+)
// Expected: Should handle large volume of exceptions efficiently
import * from "../../lib/exceptions/Exception.mt";

class CountException extends Exception {
    public int count;

    public constructor(string msg, int cnt): super(msg) {
        count = cnt;
    }

    public function getCount(): int {
        return count;
    }
}

// Throw and catch many exceptions in sequence
function testManySequentialExceptions(int iterations): void {
    int caught = 0;

    int i = 0;
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
function testNestedLoopExceptions(int outer, int inner): void {
    int totalCaught = 0;

    int i = 0;
    while (i < outer) {
        int j = 0;
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
    constructor(string msg): super(msg) { }
}

class TypeBException extends Exception {
    constructor(string msg): super(msg) {  }
}

class TypeCException extends Exception {
    constructor(string msg): super(msg) {  }
}

function testMultipleCatchTypes(int iterations): void {
    int caughtA = 0;
    int caughtB = 0;
    int caughtC = 0;

    int i = 0;
    while (i < iterations) {
        try {
            int mod = i % 3;
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
function testManyExceptionsWithFinally(int iterations): void {
    int caught = 0;
    int finallyCalled = 0;

    int i = 0;
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
testManySequentialExceptions(1000);

print("---");
print("Test 2: 50x20 nested loop exceptions (1000 total)");
testNestedLoopExceptions(50, 20);

print("---");
print("Test 3: 1000 exceptions with multiple catch types");
testMultipleCatchTypes(1000);

print("---");
print("Test 4: 500 exceptions with finally blocks");
testManyExceptionsWithFinally(500);

print("---");
print("Many exceptions test completed successfully!");
