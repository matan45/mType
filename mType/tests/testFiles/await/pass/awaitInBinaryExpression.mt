// Test await in binary expressions

import { Int } from "../../lib/primitives/Int.mt";

print("=== Await in Binary Expression Test ===");

function async getLeft(): Promise<Int> {
    print("Getting left operand");
    return new Int(100);
}

function async getRight(): Promise<Int> {
    print("Getting right operand");
    return new Int(50);
}

function async testBinaryExpression(): Promise<Int> {
    print("Evaluating binary expression");

    // Await in binary expression
    int result = (await getLeft()) + (await getRight());
    print("Result: " + result);

    return new Int(result);
}

function async main(): Promise<Int> {
    Int r = await testBinaryExpression();
    print("Final: " + r);
    return r;
}

main();
