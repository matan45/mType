// Test: Multiple catch blocks with unrelated exception types (valid)
import * from "../../lib/exceptions/Exception.mt";
import * from "../../lib/exceptions/RuntimeException.mt";
import * from "../../lib/exceptions/IllegalArgumentException.mt";
import * from "../../lib/exceptions/NullPointerException.mt";

class CustomException extends Exception {
    public constructor(string msg) : super(msg) {}

    public function toString(): string {
        return "CustomException: " + message;
    }
}

class AnotherCustomException extends Exception {
    public constructor(string msg) : super(msg) {}

    public function toString(): string {
        return "AnotherCustomException: " + message;
    }
}

function testUnrelatedCatchBlocks(int testCase): void {
    try {
        if (testCase == 1) {
            throw new CustomException("Custom error");
        } else if (testCase == 2) {
            throw new AnotherCustomException("Another custom error");
        } else if (testCase == 3) {
            throw new IllegalArgumentException("Invalid argument");
        } else {
            throw new NullPointerException("Null pointer");
        }
    } catch (CustomException ce) {
        print("Caught CustomException: " + ce.getMessage());
    } catch (AnotherCustomException ace) {
        print("Caught AnotherCustomException: " + ace.getMessage());
    } catch (IllegalArgumentException iae) {
        print("Caught IllegalArgumentException: " + iae.getMessage());
    } catch (NullPointerException npe) {
        print("Caught NullPointerException: " + npe.getMessage());
    }
}

print("Testing unrelated catch blocks");
testUnrelatedCatchBlocks(1);
testUnrelatedCatchBlocks(2);
testUnrelatedCatchBlocks(3);
testUnrelatedCatchBlocks(4);
print("Test completed");
