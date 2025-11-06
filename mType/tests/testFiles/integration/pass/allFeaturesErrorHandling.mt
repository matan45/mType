// Test: Exception handling across all features
// @Script

import "modules/ErrorHandlingInterfaces.mt";

class SafeOperation<T> implements AsyncFallible<T, String>, AsyncRetryable {
    private () -> T operation;
    private (String) -> T fallback;
    private Bool shouldFail;

    constructor(() -> T op, (String) -> T fb, Bool fail) {
        this.operation = op;
        this.fallback = fb;
        this.shouldFail = fail;
    }

    function async execute() : Promise<T> {
        await delay(5);
        if (this.shouldFail) {
            throw "Operation failed";
        }
        return this.operation();
    }

    function async onError(String error) : Promise<T> {
        await delay(5);
        print("Handling error: " + error);
        return this.fallback(error);
    }

    function async retry(Int maxAttempts) : Promise<Bool> {
        Int attempts = 0;
        while (attempts < maxAttempts) {
            try {
                await delay(5);
                await this.execute();
                return true;
            } catch (String e) {
                print("Attempt " + (attempts + 1).toString() + " failed");
                attempts = attempts + 1;
            }
        }
        return false;
    }

    function setShouldFail(Bool fail) : void {
        this.shouldFail = fail;
    }
}

function async safeExecute<T>(
    AsyncFallible<T, String> operation,
    (String) -> T errorHandler
) : Promise<T> {
    try {
        return await operation.execute();
    } catch (String e) {
        return await operation.onError(e);
    }
}

function async delay(Int ms) : Promise<void> {
    // Simulated delay
}

function async main() : Promise<void> {
    // Test successful operation
    SafeOperation<String> successOp = new SafeOperation<String>(
        () : String => { return "Success"; },
        (String e) : String => { return "Fallback"; },
        false
    );

    String result1 = await safeExecute<String>(
        successOp,
        (String e) : String => { return "Error: " + e; }
    );
    print("Result 1: " + result1);
    assert(result1 == "Success", "Should execute successfully");

    // Test failing operation with error handler
    SafeOperation<String> failOp = new SafeOperation<String>(
        () : String => { return "Success"; },
        (String e) : String => { return "Recovered: " + e; },
        true
    );

    String result2 = await safeExecute<String>(
        failOp,
        (String e) : String => { return "Handled"; }
    );
    print("Result 2: " + result2);
    assert(result2 == "Recovered: Operation failed", "Should handle error");

    // Test retry
    SafeOperation<Int> retryOp = new SafeOperation<Int>(
        () : Int => { return 42; },
        (String e) : Int => { return -1; },
        true
    );

    Bool retrySuccess = await retryOp.retry(3);
    assert(!retrySuccess, "Should fail all retries");

    retryOp.setShouldFail(false);
    Bool retrySuccess2 = await retryOp.retry(3);
    assert(retrySuccess2, "Should succeed on retry");

    print("Error handling test passed");
}
