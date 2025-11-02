// Error: Unhandled exception in async function

class UnhandledException {
    string message;

    public constructor(string msg) {
        this.message = msg;
    }

    public function getMessage(): string {
        return this.message;
    }
}

class Result {
    int value;

    public constructor(int v) {
        this.value = v;
    }

    public function getValue(): int {
        return this.value;
    }
}

function async throwUnhandled(): Promise<Result> {
    print("Throwing unhandled exception");
    // This exception is not caught
    throw new UnhandledException("This will not be caught");
    Result r = new Result(100);
    return r;
}

function async callThrowing(): Promise<Result> {
    print("Calling throwing function without try-catch");
    // ERROR: No exception handling
    Result r = await throwUnhandled();
    return r;
}

function async main(): Promise<Result> {
    Result r = await callThrowing();
    return r;
}

main();
