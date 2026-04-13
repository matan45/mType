import * from "../../lib/exceptions/Exception.mt";

function riskyOperation(): int {
    throw new Exception("Something went wrong");
}

try {
    int result = riskyOperation();
    print("Should not reach here");
} catch (Exception e) {
    print("Caught: " + e.getMessage());
}
print("After catch");
