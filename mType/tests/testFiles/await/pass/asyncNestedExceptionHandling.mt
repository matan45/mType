// Test exception handling across multiple async call levels

import { Int } from "../../lib/primitives/Int.mt";
import * from "../../lib/exceptions/Exception.mt";

print("=== Nested Async Exception Handling Test ===");

class Error extends Exception {

    public constructor(string m): super(m) {
        
    }

   
}

class Value {
    int num;

    public constructor(int n) {
        this.num = n;
    }

    public function getNum(): int {
        return this.num;
    }
}

function async level3(): Promise<Value> {
    print("Level 3: throwing error");
    throw new Error("Level 3 error");
    Value v = new Value(3);
    return v;
}

function async level2(): Promise<Value> {
    print("Level 2: calling level 3");
    Value v = await level3();
    print("Level 2: got value");
    return v;
}

function async level1(): Promise<Value> {
    print("Level 1: calling level 2 with try-catch");

    Value result;
    try {
        result = await level2();
        print("Level 1: success");
    } catch (Error e) {
        print("Level 1: caught error - " + e.getMessage());
        result = new Value(100);
    }

    return result;
}

function async main(): Promise<Int> {
    Value v = await level1();
    print("Main: final value " + v.getNum());
    return new Int(v.getNum());
}

main();
