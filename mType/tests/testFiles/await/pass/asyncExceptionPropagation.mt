// Test exception propagation across async function boundaries

import { Int } from "../../lib/primitives/Int.mt";

print("=== Async Exception Propagation Test ===");

class CustomException {
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

function async throwingAsync(): Promise<Result> {
    print("About to throw exception");
    throw new CustomException("Async function error");
    Result r = new Result(999);
    return r;
}

function async catchingAsync(): Promise<Result> {
    print("Attempting to call throwing function");

    Result res;
    try {
        res = await throwingAsync();
        print("This should not print");
    } catch (CustomException e) {
        print("Caught exception: " + e.getMessage());
        res = new Result(100);
    }

    print("Returning result: " + res.getValue());
    return res;
}

function async main(): Promise<Int> {
    Result finalResult = await catchingAsync();
    print("Final value: " + finalResult.getValue());
    return new Int(finalResult.getValue());
}

main();
