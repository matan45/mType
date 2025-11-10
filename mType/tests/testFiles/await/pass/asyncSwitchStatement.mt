// Test async function with switch statement (if supported)

import { Int } from "../../lib/primitives/Int.mt";
import { String } from "../../lib/primitives/String.mt";

print("=== Async Switch Statement Test ===");

function async getOperation(int opCode): Promise<String> {
    print("Getting operation for code: " + opCode);

    // Using if-else chain as switch alternative
    if (opCode == 1) {
        return new String("ADD");
    } else if (opCode == 2) {
        return new String("SUBTRACT");
    } else if (opCode == 3) {
        return new String("MULTIPLY");
    } else if (opCode == 4) {
        return new String("DIVIDE");
    } else {
        return new String("UNKNOWN");
    }
}

function async performOperation(int a, int b, int opCode): Promise<Int> {
    string op = (await getOperation(opCode)).getValue();
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
