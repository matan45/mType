// Test: Multiple cleanup failures with exception handling
// Expected: Should handle multiple cleanup failures gracefully
import * from "../../lib/exceptions/Exception.mt";

class ResourceException extends Exception {
    constructor(string message): super(message) {
    }
}

class Resource {
    public string name;
    public bool shouldFailOnClose;

    public constructor(string resourceName, bool failOnClose) {
        name = resourceName;
        shouldFailOnClose = failOnClose;
        print("Acquired resource: " + name);
    }

    public function close(): void {
        if (shouldFailOnClose) {
            print("Failed to close resource: " + name);
            throw new ResourceException("Cleanup failed for " + name);
        }
        print("Successfully closed resource: " + name);
    }
}

function testMultipleCleanupFailures(): void {
    Resource r1 = null;
    Resource r2 = null;
    Resource r3 = null;

    try {
        r1 = new Resource("Connection1", false);
        r2 = new Resource("Connection2", true);
        r3 = new Resource("Connection3", true);

        print("Performing operations...");
        throw new Exception("Operation failed");
    } catch (Exception e) {
        print("Caught operation exception: " + e.getMessage());
    } finally {
        print("Cleaning up resources...");

        // Cleanup r1 (should succeed)
        if (r1 != null) {
            try {
                r1.close();
            } catch (ResourceException re) {
                print("Cleanup error for r1: " + re.getMessage());
            }
        }

        // Cleanup r2 (will fail)
        if (r2 != null) {
            try {
                r2.close();
            } catch (ResourceException re) {
                print("Cleanup error for r2: " + re.getMessage());
            }
        }

        // Cleanup r3 (will fail)
        if (r3 != null) {
            try {
                r3.close();
            } catch (ResourceException re) {
                print("Cleanup error for r3: " + re.getMessage());
            }
        }

        print("All cleanup attempts completed");
    }
}

function main(): void {
    print("Testing multiple cleanup failures");
    testMultipleCleanupFailures();
    print("Test completed successfully");
}

main();
