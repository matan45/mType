// Test rethrowing exceptions across async boundaries

import { Int } from "../../lib/primitives/Int.mt";
import * from "../../lib/exceptions/Exception.mt";

print("=== Async Rethrow Exception Test ===");

class AppError extends Exception {

    public constructor(string msg): super(msg) {
        
    }

    
}

class Data {
    int value;

    public constructor(int v) {
        this.value = v;
    }

    public function getValue(): int {
        return this.value;
    }
}

function async innerFunction(): Promise<Data> {
    print("Inner: throwing error");
    throw new AppError("Inner error");
    Data d = new Data(1);
    return d;
}

function async middleFunction(): Promise<Data> {
    print("Middle: calling inner");

    Data result;
    try {
        result = await innerFunction();
    } catch (AppError e) {
        print("Middle: caught error - " + e.getMessage());
        print("Middle: rethrowing");
        throw e;
    }

    return result;
}

function async outerFunction(): Promise<Data> {
    print("Outer: calling middle");

    Data result;
    try {
        result = await middleFunction();
    } catch (AppError e) {
        print("Outer: caught rethrown error - " + e.getMessage());
        result = new Data(500);
    }

    return result;
}

function async main(): Promise<Int> {
    Data d = await outerFunction();
    print("Final: " + d.getValue());
    return new Int(d.getValue());
}

main();
