// Test async function with switch statement (if supported)

import { Int } from "../../lib/primitives/Int.mt";

print("=== Async Switch Statement Test ===");

function async getOperation(int opCode): Promise<string> {
    print("Getting operation for code: " + opCode);

    // Using if-else chain as switch alternative
    if (opCode == 1) {
        return "ADD";
    } else if (opCode == 2) {
        return "SUBTRACT";
    } else if (opCode == 3) {
        return "MULTIPLY";
    } else if (opCode == 4) {
        return "DIVIDE";
    } else {
        return "UNKNOWN";
    }
}

function async performOperation(int a, int b, int opCode): Promise<Int> {
    string op = await getOperation(opCode);
    print("Operation: " + op);

    int result = 0;

    if (op == "ADD") {
        result = a + b;
    } else if (op == "SUBTRACT") {
        result = a - b;
    } else if (op == "MULTIPLY") {
        result = a * b;
    } else if (op == "DIVIDE") {
        result = a / b;
    } else {
        result = 0;
    }

    print("Result: " + result);
    return new Int(result);
}

function async main(): Promise<Int> {
    Int r1 = await performOperation(10, 5, 1);
    print("10 + 5 = " + r1);

    Int r2 = await performOperation(10, 5, 2);
    print("10 - 5 = " + r2);

    Int r3 = await performOperation(10, 5, 3);
    print("10 * 5 = " + r3);

    return r3;
}

main();
