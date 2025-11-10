// Test await in complex nested expressions

import { Int } from "../../lib/primitives/Int.mt";

print("=== Await in Complex Expression Test ===");

function async getA(): Promise<Int> {
    print("Get A");
    return new Int(5);
}

function async getB(): Promise<Int> {
    print("Get B");
    return new Int(10);
}

function async getC(): Promise<Int> {
    print("Get C");
    return new Int(3);
}

function async testComplexExpression(): Promise<Int> {
    print("Evaluating complex expression");

    // Complex expression: ((a + b) * c) / 2
    int a = (await getA()).getValue();
    int b = (await getB()).getValue();
    int c = (await getC()).getValue();

    int result = ((a + b) * c) / 2;
    print("Result: " + result);

    return new Int(result);
}

function async main(): Promise<Int> {
    Int r = await testComplexExpression();
    print("Final: " + r);
    return r;
}

main();
