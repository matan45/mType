// Nested exception handling test
import * from "../../lib/primitives/String.mt";
import * from "../../lib/exceptions/Exception.mt";

interface Function {
    function apply(int x) : String;
}

print("=== Nested Try-Catch Test ===");

Function nestedHandler = x -> {
    String result = "";
    try {
        result = result + "Outer-try ";
        try {
            result = result + "Inner-try ";
            if (x == 1) {
                throw new Exception("Inner-exception");
            }
            if (x == 2) {
                throw new Exception("Outer-exception");
            }
            result = result + "No-exception ";
        } catch (Exception e) {
            if (e.getMessage() == "Inner-exception") {
                result = result + "Inner-catch ";
            } else {
                throw e;  // Re-throw to outer
            }
        }
        result = result + "After-inner ";
    } catch (Exception e) {
        result = result + "Outer-catch ";
    }
    return result;
};

print("x=0: " + nestedHandler.apply(0));
print("x=1: " + nestedHandler.apply(1));
print("x=2: " + nestedHandler.apply(2));

// Triple nesting
Function tripleNested = x -> {
    try {
        try {
            try {
                if (x == 1) {
                    throw new Exception("Level-1");
                } else if (x == 2) {
                    throw new Exception("Level-2");
                } else if (x == 3) {
                    throw new Exception("Level-3");
                }
                return "No error";
            } catch (Exception e) {
                if (e.getMessage() == "Level-1") {
                    return "Caught at level 1";
                }
                throw e;
            }
        } catch (Exception e) {
            if (e.getMessage() == "Level-2") {
                return "Caught at level 2";
            }
            throw e;
        }
    } catch (Exception e) {
        return "Caught at level 3";
    }
};

print("Triple(0): " + tripleNested.apply(0));
print("Triple(1): " + tripleNested.apply(1));
print("Triple(2): " + tripleNested.apply(2));
print("Triple(3): " + tripleNested.apply(3));

print("Nested try-catch complete");
