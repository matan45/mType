// Test: Override with narrower (covariant) exception types
// Expected: Should compile and run successfully
import * from "../../lib/exceptions/Exception.mt";
import * from "../../lib/exceptions/RuntimeException.mt";
import * from "../../lib/exceptions/IllegalArgumentException.mt";
import * from "../../lib/primitives/String.mt";

// Base class with general exception
class BaseService {
    public function execute(string command): string {
        print("BaseService executing: " + command);
        if (command == "fail") {
            throw new Exception("Base execution failed");
        }
        return "Base result: " + command;
    }
}

// Derived class narrows to RuntimeException (covariant)
class DerivedService extends BaseService {
    public function execute(string command): string {
        print("DerivedService executing: " + command);
        if (command == "runtime_fail") {
            throw new RuntimeException("Derived runtime failure");
        }
        return "Derived result: " + command;
    }
}

// Further derived class narrows to IllegalArgumentException
class SpecializedService extends DerivedService {
    public function execute(string command): string {
        print("SpecializedService executing: " + command);
        if (command == "illegal") {
            throw new IllegalArgumentException("Specialized illegal argument");
        }
        return "Specialized result: " + command;
    }
}

// Class that removes throws clause entirely (ultimate narrowing)
class SafeService extends BaseService {
    public function execute(string command): string {
        print("SafeService executing: " + command);
        return "Safe result: " + command;
    }
}

function testPolymorphicExceptionHandling(): void {
    print("Testing polymorphic exception handling:");

    BaseService[] services = new BaseService[4];
    services[0] = new BaseService();
    services[1] = new DerivedService();
    services[2] = new SpecializedService();
    services[3] = new SafeService();

    for (int i = 0; i < 4; i++) {
        try {
            string result = services[i].execute("test" + i);
            print("Result " + i + ": " + result);
        } catch (Exception e) {
            print("Caught exception " + i + ": " + e.getMessage());
        }
    }
}

function testNarrowingExceptions(): void {
    print("Testing exception narrowing:");

    DerivedService derived = new DerivedService();
    try {
        string result = derived.execute("runtime_fail");
        print("Should not reach here");
    } catch (RuntimeException e) {
        print("Caught narrowed RuntimeException: " + e.getMessage());
    }

    SpecializedService specialized = new SpecializedService();
    try {
        string result = specialized.execute("illegal");
        print("Should not reach here");
    } catch (IllegalArgumentException e) {
        print("Caught narrowest IllegalArgumentException: " + e.getMessage());
    }
}

function testSafeOverride(): void {
    print("Testing safe override (no exceptions):");

    SafeService safe = new SafeService();
    string result = safe.execute("always_succeeds");
    print("Safe execution: " + result);
}

print("=== Test 1: Polymorphic Exception Handling ===");
testPolymorphicExceptionHandling();

print("");
print("=== Test 2: Exception Narrowing ===");
testNarrowingExceptions();

print("");
print("=== Test 3: Safe Override ===");
testSafeOverride();

print("");
print("Covariant exception types test passed!");
