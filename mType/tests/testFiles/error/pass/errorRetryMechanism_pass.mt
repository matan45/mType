// Test: Automatic retry mechanism on exception
// Expected: Should retry N times before failing or succeeding
import * from "../../lib/exceptions/Exception.mt";
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

// Custom exception for retry testing
class RetryableException extends Exception {
    public constructor(string msg) {
        super(msg);
    }
}

// Function that fails the first N-1 times, succeeds on the Nth attempt
int attemptCount = 0;
function unstableOperation(int maxAttempts): string {
    attemptCount = attemptCount + 1;
    print("Attempt #" + attemptCount);

    if (attemptCount < maxAttempts) {
        throw new RetryableException("Operation failed, attempt " + attemptCount);
    }

    return "Success after " + attemptCount + " attempts";
}

// Generic retry mechanism
function retry<T>(function(): T operation, int maxRetries): T {
    int attempt = 0;

    while (attempt < maxRetries) {
        try {
            T result = operation();
            print("Operation succeeded on attempt " + (attempt + 1));
            return result;
        } catch (RetryableException e) {
            attempt = attempt + 1;
            print("Caught: " + e.getMessage());

            if (attempt >= maxRetries) {
                print("Max retries reached, giving up");
                throw e;
            }

            print("Retrying... (" + attempt + "/" + maxRetries + ")");
        }
    }

    throw new Exception("Unexpected retry loop exit");
}

// Test 1: Operation succeeds on 3rd attempt (within retry limit)
print("=== Test 1: Success after retries ===");
attemptCount = 0;
try {
    string result = retry(() => unstableOperation(3), 5);
    print("Final result: " + result);
} catch (Exception e) {
    print("Test 1 failed: " + e.getMessage());
}

// Test 2: Operation exceeds retry limit
print("\n=== Test 2: Exceeds retry limit ===");
attemptCount = 0;
try {
    string result = retry(() => unstableOperation(10), 3);
    print("Final result: " + result);
} catch (RetryableException e) {
    print("Operation failed after retries: " + e.getMessage());
}

// Test 3: Immediate success (no retry needed)
print("\n=== Test 3: Immediate success ===");
attemptCount = 0;
try {
    string result = retry(() => unstableOperation(1), 5);
    print("Final result: " + result);
} catch (Exception e) {
    print("Test 3 failed: " + e.getMessage());
}

// Test 4: Exponential backoff simulation (with counter)
print("\n=== Test 4: Retry with delay tracking ===");
attemptCount = 0;
int delayMs = 100;

function retryWithBackoff(function(): string operation, int maxRetries): string {
    int attempt = 0;
    int currentDelay = delayMs;

    while (attempt < maxRetries) {
        try {
            return operation();
        } catch (RetryableException e) {
            attempt = attempt + 1;

            if (attempt >= maxRetries) {
                throw e;
            }

            print("Waiting " + currentDelay + "ms before retry");
            currentDelay = currentDelay * 2;  // Exponential backoff
        }
    }

    throw new Exception("Unexpected exit");
}

try {
    string result = retryWithBackoff(() => unstableOperation(3), 5);
    print("Backoff result: " + result);
} catch (Exception e) {
    print("Backoff failed: " + e.getMessage());
}

print("\nRetry mechanism test completed!");
