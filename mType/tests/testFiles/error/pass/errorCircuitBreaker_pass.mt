// Test: Circuit breaker pattern (fail fast after threshold)
// Expected: Should open circuit after threshold failures
import * from "../../lib/exceptions/Exception.mt";
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

// Custom exceptions
class ServiceException extends Exception {
    public constructor(string msg) {
        super(msg);
    }
}

class CircuitOpenException extends Exception {
    public constructor(string msg) {
        super(msg);
    }
}

// Circuit breaker states
class CircuitBreaker {
    private int failureCount;
    private int failureThreshold;
    private int successCount;
    private int resetThreshold;
    private bool isOpen;
    private string stateName;

    public constructor(int threshold, int resetCount) {
        failureCount = 0;
        failureThreshold = threshold;
        successCount = 0;
        resetThreshold = resetCount;
        isOpen = false;
        stateName = "CLOSED";
    }

    public function getState(): string {
        return stateName;
    }

    public function getFailureCount(): int {
        return failureCount;
    }

    public function execute(function(): string operation): string {
        // If circuit is open, fail fast
        if (isOpen) {
            print("Circuit is OPEN - failing fast");
            throw new CircuitOpenException("Circuit breaker is open");
        }

        // Try to execute the operation
        try {
            string result = operation();

            // Success - reset failure count
            failureCount = 0;
            successCount = successCount + 1;
            print("Operation succeeded (success count: " + successCount + ")");

            // If we're in half-open state and got enough successes, close the circuit
            if (stateName == "HALF_OPEN" && successCount >= resetThreshold) {
                stateName = "CLOSED";
                successCount = 0;
                print("Circuit state: HALF_OPEN -> CLOSED");
            }

            return result;
        } catch (ServiceException e) {
            failureCount = failureCount + 1;
            print("Operation failed (failure " + failureCount + "/" + failureThreshold + ")");

            // Check if we should open the circuit
            if (failureCount >= failureThreshold) {
                isOpen = true;
                stateName = "OPEN";
                print("Circuit state: -> OPEN (threshold reached)");
            }

            throw e;
        }
    }

    public function attemptReset(): void {
        if (isOpen) {
            isOpen = false;
            stateName = "HALF_OPEN";
            successCount = 0;
            print("Circuit state: OPEN -> HALF_OPEN (manual reset)");
        }
    }
}

// Test service that can be controlled
int callCount = 0;
bool shouldFail = true;

function simulatedService(): string {
    callCount = callCount + 1;

    if (shouldFail) {
        throw new ServiceException("Service unavailable");
    }

    return "Service response OK";
}

// Test 1: Circuit opens after threshold failures
print("=== Test 1: Circuit opens after failures ===");
CircuitBreaker cb1 = new CircuitBreaker(3, 2);
callCount = 0;
shouldFail = true;

int i = 0;
while (i < 5) {
    try {
        string result = cb1.execute(() => simulatedService());
        print("Call " + (i + 1) + ": " + result);
    } catch (CircuitOpenException e) {
        print("Call " + (i + 1) + ": Circuit open - " + e.getMessage());
    } catch (ServiceException e) {
        print("Call " + (i + 1) + ": Service failed - " + e.getMessage());
    }
    i = i + 1;
}
print("Circuit state: " + cb1.getState());

// Test 2: Circuit can be reset and closed again
print("\n=== Test 2: Circuit reset and recovery ===");
CircuitBreaker cb2 = new CircuitBreaker(2, 3);
callCount = 0;
shouldFail = true;

// Fail enough to open circuit
i = 0;
while (i < 3) {
    try {
        cb2.execute(() => simulatedService());
    } catch (Exception e) {
        print("Failure " + (i + 1) + ": " + e.getMessage());
    }
    i = i + 1;
}

print("State after failures: " + cb2.getState());

// Attempt reset and start succeeding
cb2.attemptReset();
shouldFail = false;

i = 0;
while (i < 4) {
    try {
        string result = cb2.execute(() => simulatedService());
        print("Success " + (i + 1) + ": " + result);
    } catch (Exception e) {
        print("Error " + (i + 1) + ": " + e.getMessage());
    }
    i = i + 1;
}

print("Final state: " + cb2.getState());

// Test 3: Half-open state can fail and reopen
print("\n=== Test 3: Half-open state failure ===");
CircuitBreaker cb3 = new CircuitBreaker(2, 2);
callCount = 0;
shouldFail = true;

// Open the circuit
i = 0;
while (i < 2) {
    try {
        cb3.execute(() => simulatedService());
    } catch (Exception e) {
        // Ignore
    }
    i = i + 1;
}

print("State: " + cb3.getState());

// Try to reset but service still fails
cb3.attemptReset();
print("State after reset: " + cb3.getState());

try {
    cb3.execute(() => simulatedService());
} catch (ServiceException e) {
    print("Half-open test failed: " + e.getMessage());
}

print("State after failure in half-open: " + cb3.getState());

// Try again - should fail fast
try {
    cb3.execute(() => simulatedService());
} catch (CircuitOpenException e) {
    print("Failed fast: " + e.getMessage());
}

print("\nCircuit breaker test completed!");
