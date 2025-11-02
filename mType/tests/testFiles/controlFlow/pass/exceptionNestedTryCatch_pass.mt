// Test nested try-catch blocks
import * from "../../lib/exceptions/Exception.mt";

function testNestedTryCatch(): void {
    try {
        print("Outer try block");

        try {
            print("Inner try block");
            throw new Exception("Inner exception");
        } catch (Exception e) {
            print("Inner catch: " + e.getMessage());
            throw new Exception("Re-thrown from inner");
        }

    } catch (Exception e) {
        print("Outer catch: " + e.getMessage());
    }
}

function testNestedNoPropagate(): void {
    try {
        print("Outer try 2");

        try {
            print("Inner try 2");
            throw new Exception("Inner only");
        } catch (Exception e) {
            print("Inner catch 2: " + e.getMessage());
        }

        print("After inner try-catch");

    } catch (Exception e) {
        print("Should not reach outer catch");
    }
}

function main(): void {
    print("Testing nested try-catch blocks");

    testNestedTryCatch();
    testNestedNoPropagate();

    print("Test completed");
}

main();
