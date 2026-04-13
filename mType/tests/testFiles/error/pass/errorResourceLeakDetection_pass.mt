// Test: Resource leak tracking and detection
// Expected: Should track and report resource leaks
import * from "../../lib/exceptions/Exception.mt";

class ResourceTracker {
    public static int totalAllocated = 0;
    public static int totalReleased = 0;

    public static function allocate(): void {
        totalAllocated = totalAllocated + 1;
    }

    public static function release(): void {
        totalReleased = totalReleased + 1;
    }

    public static function getLeakCount(): int {
        return totalAllocated - totalReleased;
    }

    public static function reset(): void {
        totalAllocated = 0;
        totalReleased = 0;
    }

    public static function report(): void {
        print("Resource Report:");
        print("  Allocated: " + totalAllocated);
        print("  Released: " + totalReleased);
        print("  Leaked: " + getLeakCount());
    }
}

class TrackedResource {
    public string name;
    public Bool isReleased;

    public constructor(string resourceName) {
        name = resourceName;
        isReleased = false;
        ResourceTracker::allocate();
        print("Allocated: " + name);
    }

    public function use(): void {
        if (isReleased) {
            throw new Exception("Using released resource: " + name);
        }
        print("Using: " + name);
    }

    public function release(): void {
        if (!isReleased) {
            ResourceTracker::release();
            isReleased = true;
            print("Released: " + name);
        }
    }
}

function testNoLeak(): void {
    print("Test 1: No resource leaks");
    ResourceTracker::reset();

    TrackedResource r1 = new TrackedResource("Resource1");
    TrackedResource r2 = new TrackedResource("Resource2");

    r1.use();
    r2.use();

    r1.release();
    r2.release();

    ResourceTracker::report();
}

function testWithLeak(): void {
    print("Test 2: Resource leak detection");
    ResourceTracker::reset();

    TrackedResource r1 = new TrackedResource("LeakyResource1");
    TrackedResource r2 = new TrackedResource("LeakyResource2");
    TrackedResource r3 = new TrackedResource("ProperResource");

    r1.use();
    r2.use();
    r3.use();

    // Only release r3, r1 and r2 leak
    r3.release();

    ResourceTracker::report();
}

function testExceptionLeak(): void {
    print("Test 3: Leak on exception without finally");
    ResourceTracker::reset();

    try {
        TrackedResource r1 = new TrackedResource("ExceptionResource1");
        TrackedResource r2 = new TrackedResource("ExceptionResource2");

        r1.use();
        throw new Exception("Operation failed");
        r1.release();
        r2.release();
    } catch (Exception e) {
        print("Caught: " + e.getMessage());
    }

    ResourceTracker::report();
}

function testExceptionNoLeak(): void {
    print("Test 4: No leak with finally block");
    ResourceTracker::reset();

    TrackedResource? r1 = null;
    TrackedResource? r2 = null;

    try {
        r1 = new TrackedResource("SafeResource1");
        r2 = new TrackedResource("SafeResource2");

        if (r1 != null) {
            r1.use();
        }
        throw new Exception("Operation failed");
    } catch (Exception e) {
        print("Caught: " + e.getMessage());
    } finally {
        if (r1 != null) {
            r1.release();
        }
        if (r2 != null) {
            r2.release();
        }
    }

    ResourceTracker::report();
}

function testPartialLeak(): void {
    print("Test 5: Partial leak detection");
    ResourceTracker::reset();

    TrackedResource r1 = new TrackedResource("Partial1");
    TrackedResource r2 = new TrackedResource("Partial2");
    TrackedResource r3 = new TrackedResource("Partial3");
    TrackedResource r4 = new TrackedResource("Partial4");

    r1.use();
    r2.use();
    r3.use();
    r4.use();

    // Release only r1 and r3
    r1.release();
    r3.release();
    // r2 and r4 leak

    ResourceTracker::report();
}

function testNestedLeakTracking(): void {
    print("Test 6: Nested scope leak tracking");
    ResourceTracker::reset();

    TrackedResource? outer = null;

    try {
        outer = new TrackedResource("OuterResource");
        TrackedResource? inner1 = null;
        TrackedResource? inner2 = null;

        try {
            inner1 = new TrackedResource("InnerResource1");
            inner2 = new TrackedResource("InnerResource2");

            if (inner1 != null) {
                inner1.use();
            }
            if (inner2 != null) {
                inner2.use();
            }

            // Release only inner1
            if (inner1 != null) {
                inner1.release();
            }
            // inner2 leaks
        } finally {
            // Note: inner2 not released here
        }

        if (outer != null) {
            outer.use();
            outer.release();
        }
    } catch (Exception e) {
        print("Error: " + e.getMessage());
    }

    ResourceTracker::report();
}

function main(): void {
    print("Testing resource leak detection");
    testNoLeak();
    print("---");
    testWithLeak();
    print("---");
    testExceptionLeak();
    print("---");
    testExceptionNoLeak();
    print("---");
    testPartialLeak();
    print("---");
    testNestedLeakTracking();
    print("Leak detection test completed");
}

main();
