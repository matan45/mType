// Test: Exception during module initialization is properly caught
import * from "../../lib/exceptions/Exception.mt";
import * from "./modules/ThrowingModule.mt";

function testModuleInitException(): void {
    try {
        exceptionThrow();
        print("Should not reach here - module threw during init");
    } catch (Exception e) {
        print("Caught exception during module import: " + e.getMessage());
    }
}

function testMultipleImportAttempts(): void {
    print("First import attempt:");
    try {
        exceptionThrow();
        print("Import succeeded (unexpected)");
    } catch (Exception e) {
        print("First attempt caught: " + e.getMessage());
    }

    print("Second import attempt:");
    try {
        exceptionThrow();
        print("Import succeeded (unexpected)");
    } catch (Exception e) {
        print("Second attempt caught: " + e.getMessage());
    }
}

function testNestedModuleException(): void {
    try {
        try {
            exceptionThrow();
            print("Inner: Should not reach");
        } catch (Exception inner) {
            print("Inner catch: " + inner.getMessage());
            throw inner;
        }
    } catch (Exception outer) {
        print("Outer catch: Re-caught exception");
    }
}

print("Testing module initialization exceptions");
testModuleInitException();
testMultipleImportAttempts();
testNestedModuleException();
print("All module initialization exception tests completed");
