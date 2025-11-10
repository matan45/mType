// Test await as function argument

import { Int } from "../../lib/primitives/Int.mt";

print("=== Await as Argument Test ===");

class Adder {
    public function add(int a, int b): int {
        return a + b;
    }

    public function addThree(int a, int b, int c): int {
        return a + b + c;
    }
}

function async getArg1(): Promise<Int> {
    print("Getting arg 1");
    return new Int(10);
}

function async getArg2(): Promise<Int> {
    print("Getting arg 2");
    return new Int(20);
}

function async getArg3(): Promise<Int> {
    print("Getting arg 3");
    return new Int(30);
}

function async testAwaitAsArgs(): Promise<Int> {
    print("Testing await as arguments");

    Adder adder = new Adder();

    // Multiple awaits as arguments
    int result = adder.addThree((await getArg1()).getValue(),
                                 (await getArg2()).getValue(),
                                 (await getArg3()).getValue());
    print("Result: " + result);

    return new Int(result);
}

function async main(): Promise<Int> {
    Int r = await testAwaitAsArgs();
    print("Final: " + r);
    return r;
}

main();
