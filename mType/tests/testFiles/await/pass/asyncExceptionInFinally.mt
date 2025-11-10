// Test exception thrown in finally block of async function

import { Int } from "../../lib/primitives/Int.mt";
import * from "../../lib/exceptions/Exception.mt";

print("=== Exception in Finally Block Test ===");

class FinallyException extends Exception {
   
    public constructor(string m): super(m) {  }
    
}

class Result {
    int value;
    public constructor(int v) { this.value = v; }
    public function getValue(): int { return this.value; }
}

function async functionWithFinallyException(): Promise<Result> {
    print("Entering function");

    Result res = new Result(50);
    try {
        print("In try block");
        res = new Result(100);
    } finally {
        print("In finally block - throwing exception");
        throw new FinallyException("Finally exception");
    }

    print("This line should not execute");
    return res;
}

function async catchFinallyException(): Promise<Result> {
    print("Calling function with finally exception");

    Result res;
    try {
        res = await functionWithFinallyException();
    } catch (FinallyException e) {
        print("Caught finally exception: " + e.getMessage());
        res = new Result(999);
    }

    return res;
}

function async main(): Promise<Int> {
    Result r = await catchFinallyException();
    print("Final result: " + r.getValue());
    return new Int(r.getValue());
}

main();
