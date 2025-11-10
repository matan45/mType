// Test: Finally block for guaranteed cleanup
// Expected: Finally blocks should always execute for resource cleanup
import * from "../../lib/exceptions/Exception.mt";

class CleanupResource {
    public string name;
    public bool cleaned;

    public constructor(string resourceName) {
        name = resourceName;
        cleaned = false;
        print("Created: " + name);
    }

    public function cleanup(): void {
        if (!cleaned) {
            print("Cleaning up: " + name);
            cleaned = true;
        }
    }

    public function isClean(): bool {
        return cleaned;
    }
}

function testFinallyWithoutException(): void {
    print("Test 1: Finally executes without exception");
    CleanupResource resource = null;

    try {
        resource = new CleanupResource("NormalResource");
        print("Performing normal operation");
    } finally {
        if (resource != null) {
            resource.cleanup();
        }
        print("Finally block executed");
    }
}

function testFinallyWithException(): void {
    print("Test 2: Finally executes with exception");
    CleanupResource resource = null;

    try {
        resource = new CleanupResource("ExceptionResource");
        print("Before exception");
        throw new Exception("Operation error");
        print("After exception (unreachable)");
    } catch (Exception e) {
        print("Caught: " + e.getMessage());
    } finally {
        if (resource != null) {
            resource.cleanup();
        }
        print("Finally block executed despite exception");
    }
}

function testFinallyWithReturn(): void {
    print("Test 3: Finally executes before return");
    CleanupResource resource = null;

    try {
        resource = new CleanupResource("ReturnResource");
        print("Before return");
        return;
    } finally {
        if (resource != null) {
            resource.cleanup();
        }
        print("Finally executed before return");
    }
}

function testMultipleFinally(): void {
    print("Test 4: Multiple nested finally blocks");
    CleanupResource r1 = null;
    CleanupResource r2 = null;
    CleanupResource r3 = null;

    try {
        r1 = new CleanupResource("Outer");

        try {
            r2 = new CleanupResource("Middle");

            try {
                r3 = new CleanupResource("Inner");
                print("All resources created");
                throw new Exception("Inner exception");
            } catch (Exception e) {
                print("Inner catch: " + e.getMessage());
            } finally {
                if (r3 != null) {
                    r3.cleanup();
                }
                print("Inner finally");
            }

            print("Middle scope continues");
        } finally {
            if (r2 != null) {
                r2.cleanup();
            }
            print("Middle finally");
        }

        print("Outer scope continues");
    } finally {
        if (r1 != null) {
            r1.cleanup();
        }
        print("Outer finally");
    }
}

function testFinallyWithCatchAndThrow(): void {
    print("Test 5: Finally with catch that rethrows");
    CleanupResource resource = null;

    try {
        try {
            resource = new CleanupResource("RethrowResource");
            throw new Exception("Original error");
        } catch (Exception e) {
            print("Caught original: " + e.getMessage());
            throw new Exception("Rethrown error");
        } finally {
            if (resource != null) {
                resource.cleanup();
            }
            print("Finally before rethrow");
        }
    } catch (Exception e) {
        print("Caught rethrown: " + e.getMessage());
    }
}

function testFinallyMultipleResources(): void {
    print("Test 6: Finally cleaning up multiple resources");
    CleanupResource r1 = null;
    CleanupResource r2 = null;
    CleanupResource r3 = null;
    CleanupResource r4 = null;

    try {
        r1 = new CleanupResource("Res1");
        r2 = new CleanupResource("Res2");
        r3 = new CleanupResource("Res3");
        r4 = new CleanupResource("Res4");

        print("Using all resources");
        throw new Exception("Multi-resource error");
    } catch (Exception e) {
        print("Error: " + e.getMessage());
    } finally {
        print("Cleaning all resources in finally:");
        if (r1 != null) {
            r1.cleanup();
        }
        if (r2 != null) {
            r2.cleanup();
        }
        if (r3 != null) {
            r3.cleanup();
        }
        if (r4 != null) {
            r4.cleanup();
        }
        print("All resources cleaned");
    }
}

function testFinallyWithoutCatch(): void {
    print("Test 7: Finally without catch block");
    CleanupResource resource = null;

    try {
        try {
            resource = new CleanupResource("NoCatchResource");
            print("Operation executing");
            throw new Exception("Uncaught here");
        } finally {
            if (resource != null) {
                resource.cleanup();
            }
            print("Finally executes even without catch");
        }
    } catch (Exception e) {
        print("Outer catch: " + e.getMessage());
    }
}

function testFinallyExecutionOrder(): void {
    print("Test 8: Finally execution order verification");
    CleanupResource r1 = null;
    CleanupResource r2 = null;

    try {
        r1 = new CleanupResource("First");
        try {
            r2 = new CleanupResource("Second");
            print("Both resources active");
        } finally {
            print("Inner finally (should execute first)");
            if (r2 != null) {
                r2.cleanup();
            }
        }
        print("Between finally blocks");
    } finally {
        print("Outer finally (should execute second)");
        if (r1 != null) {
            r1.cleanup();
        }
    }
    print("All finally blocks completed");
}

function main(): void {
    print("Testing finally block for guaranteed cleanup");
    testFinallyWithoutException();
    print("---");
    testFinallyWithException();
    print("---");
    testFinallyWithReturn();
    print("---");
    testMultipleFinally();
    print("---");
    testFinallyWithCatchAndThrow();
    print("---");
    testFinallyMultipleResources();
    print("---");
    testFinallyWithoutCatch();
    print("---");
    testFinallyExecutionOrder();
    print("Finally cleanup test completed");
}

main();
