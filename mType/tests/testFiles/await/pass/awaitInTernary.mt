// Test await in ternary conditional expression

import { Int } from "../../lib/primitives/Int.mt";
import { Bool } from "../../lib/primitives/Bool.mt";

print("=== Await in Ternary Test ===");

function async getTrueValue(): Promise<Int> {
    print("Getting true value");
    return new Int(100);
}

function async getFalseValue(): Promise<Int> {
    print("Getting false value");
    return new Int(200);
}

function async testTernary(Bool condition): Promise<Int> {
    print("Testing ternary with condition: " + condition.getValue());

    // Await in ternary expression
    Int result = condition.getValue() ? await getTrueValue() : await getFalseValue();
    print("Result: " + result);

    return result;
}

function async main(): Promise<Int> {
    Bool trueCondition = new Bool(true);
    Int r1 = await testTernary(trueCondition);
    print("Test 1: " + r1);

    Bool falseCondition = new Bool(false);
    Int r2 = await testTernary(falseCondition);
    print("Test 2: " + r2);

    int total = r1.getValue() + r2.getValue();
    return new Int(total);
}

main();
