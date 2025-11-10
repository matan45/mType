// Test exception propagation through call stack
import * from "../../lib/exceptions/Exception.mt";

function level3(): void {
    print("Level 3: throwing exception");
    throw new Exception("Error at level 3");
}

function level2(): void {
    print("Level 2: calling level 3");
    level3();
    print("Level 2: after level 3 (not reached)");
}

function level1(): void {
    print("Level 1: calling level 2");
    level2();
    print("Level 1: after level 2 (not reached)");
}

function testPropagation(): void {
    try {
        print("Top level: calling level 1");
        level1();
        print("Top level: after level 1 (not reached)");
    } catch (Exception e) {
        print("Caught at top level: " + e.getMessage());
    }
}

function level2WithCatch(): void {
    print("Level 2 with catch: calling level 3");
    try {
        level3();
    } catch (Exception e) {
        print("Caught at level 2: " + e.getMessage());
        throw new Exception("Re-thrown from level 2");
    }
}

function testPartialPropagation(): void {
    try {
        print("Starting partial propagation test");
        level2WithCatch();
    } catch (Exception e) {
        print("Caught at top: " + e.getMessage());
    }
}

function main(): void {
    print("Testing exception propagation");

    testPropagation();
    testPartialPropagation();

    print("Test completed");
}

main();
