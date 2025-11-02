// Test multiple exception handlers in async function chain

import { Int } from "../../lib/primitives/Int.mt";

print("=== Multiple Exception Handlers Test ===");

class ErrorA {
    string msg;
    public constructor(string m) { this.msg = m; }
    public function getMsg(): string { return this.msg; }
}

class ErrorB {
    string msg;
    public constructor(string m) { this.msg = m; }
    public function getMsg(): string { return this.msg; }
}

class Result {
    int value;
    public constructor(int v) { this.value = v; }
    public function getValue(): int { return this.value; }
}

function async throwErrorA(): Promise<Result> {
    print("Throwing ErrorA");
    throw new ErrorA("Error type A");
    Result r = new Result(1);
    return r;
}

function async throwErrorB(): Promise<Result> {
    print("Throwing ErrorB");
    throw new ErrorB("Error type B");
    Result r = new Result(2);
    return r;
}

function async handleMultipleErrors(int testCase): Promise<Result> {
    print("Test case: " + testCase);

    Result res;
    try {
        if (testCase == 1) {
            res = await throwErrorA();
        } else {
            res = await throwErrorB();
        }
    } catch (ErrorA ea) {
        print("Caught ErrorA: " + ea.getMsg());
        res = new Result(100);
    } catch (ErrorB eb) {
        print("Caught ErrorB: " + eb.getMsg());
        res = new Result(200);
    }

    return res;
}

function async main(): Promise<Int> {
    Result r1 = await handleMultipleErrors(1);
    print("Result 1: " + r1.getValue());

    Result r2 = await handleMultipleErrors(2);
    print("Result 2: " + r2.getValue());

    int total = r1.getValue() + r2.getValue();
    print("Total: " + total);

    return new Int(total);
}

main();
