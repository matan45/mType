// Test: Exception type from imported module can be caught and handled
import * from "../../lib/exceptions/Exception.mt";
import * from "./modules/CustomException.mt";

function throwCustomException(): void {
    throw new CustomException("Cross-module exception test");
}

function testCatchImportedException(): void {
    try {
        throwCustomException();
        print("Should not reach here");
    } catch (CustomException ce) {
        print("Caught CustomException: " + ce.getMessage());
    } catch (Exception e) {
        print("Caught generic Exception: " + e.getMessage());
    }
}

function testCatchAsBaseType(): void {
    try {
        throw new CustomException("Caught as base type");
    } catch (Exception e) {
        print("Caught as Exception: " + e.getMessage());
    }
}

function testMultipleModuleExceptions(): void {
    try {
        throw new CustomException("Multiple module test");
    } catch (CustomException ce) {
        print("Caught specific: " + ce.getMessage());
    } catch (Exception e) {
        print("Caught generic: " + e.getMessage());
    }
}

print("Testing cross-module exception handling");
testCatchImportedException();
testCatchAsBaseType();
testMultipleModuleExceptions();
print("All cross-module exception tests passed");
