// Test: Exception handling across all features
// @Script

import "modules/ErrorHandlingInterfaces.mt";

class SafeOperation<T> : AsyncFallible<T, String>, AsyncRetryable {
    private operation: () -> T;
    private fallback: (String) -> T;
    private shouldFail: Bool;

    constructor(op: () -> T, fb: (String) -> T, fail: Bool) {
        this.operation = op;
        this.fallback = fb;
        this.shouldFail = fail;
    }

    async execute() : Promise<T> {
        await delay(5);
        if (this.shouldFail) {
            throw "Operation failed";
        }
        return this.operation();
    }

    async onError(error: String) : Promise<T> {
        await delay(5);
        print("Handling error: " + error);
        return this.fallback(error);
    }

    async retry(maxAttempts: Int) : Promise<Bool> {
        let attempts: Int = 0;
        while (attempts < maxAttempts) {
            try {
                await delay(5);
                await this.execute();
                return true;
            } catch (e: String) {
                print("Attempt " + (attempts + 1).toString() + " failed");
                attempts = attempts + 1;
            }
        }
        return false;
    }

    setShouldFail(fail: Bool) : Void {
        this.shouldFail = fail;
    }
}

async safeExecute<T>(
    operation: AsyncFallible<T, String>,
    errorHandler: (String) -> T
) : Promise<T> {
    try {
        return await operation.execute();
    } catch (e: String) {
        return await operation.onError(e);
    }
}

async delay(ms: Int) : Promise<Void> {
    // Simulated delay
}

async main() : Promise<Void> {
    // Test successful operation
    let successOp = new SafeOperation<String>(
        () : String => { return "Success"; },
        (e: String) : String => { return "Fallback"; },
        false
    );

    let result1 = await safeExecute<String>(
        successOp,
        (e: String) : String => { return "Error: " + e; }
    );
    print("Result 1: " + result1);
    assert(result1 == "Success", "Should execute successfully");

    // Test failing operation with error handler
    let failOp = new SafeOperation<String>(
        () : String => { return "Success"; },
        (e: String) : String => { return "Recovered: " + e; },
        true
    );

    let result2 = await safeExecute<String>(
        failOp,
        (e: String) : String => { return "Handled"; }
    );
    print("Result 2: " + result2);
    assert(result2 == "Recovered: Operation failed", "Should handle error");

    // Test retry
    let retryOp = new SafeOperation<Int>(
        () : Int => { return 42; },
        (e: String) : Int => { return -1; },
        true
    );

    let retrySuccess = await retryOp.retry(3);
    assert(!retrySuccess, "Should fail all retries");

    retryOp.setShouldFail(false);
    let retrySuccess2 = await retryOp.retry(3);
    assert(retrySuccess2, "Should succeed on retry");

    print("Error handling test passed");
}
