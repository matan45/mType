// Test conditional promise creation and awaiting based on runtime values

import { Int } from "../../lib/primitives/Int.mt";
import { Bool } from "../../lib/primitives/Bool.mt";

print("=== Conditional Promises Test ===");

class ConditionalResult {
    int value;
    string path;

    public constructor(int v, string p) {
        this.value = v;
        this.path = p;
    }

    public function getValue(): int {
        return this.value;
    }

    public function getPath(): string {
        return this.path;
    }
}

function async pathA(): Promise<ConditionalResult> {
    print("Taking path A");
    ConditionalResult r = new ConditionalResult(100, "PathA");
    return r;
}

function async pathB(): Promise<ConditionalResult> {
    print("Taking path B");
    ConditionalResult r = new ConditionalResult(200, "PathB");
    return r;
}

function async conditionalExecution(Bool condition): Promise<ConditionalResult> {
    print("Checking condition: " + condition.getValue());

    ConditionalResult result;
    if (condition.getValue()) {
        result = await pathA();
    } else {
        result = await pathB();
    }

    print("Selected: " + result.getPath() + " with value " + result.getValue());
    return result;
}

function async testConditionalPromises(): Promise<Int> {
    print("Test 1: condition true");
    Bool condTrue = new Bool(true);
    ConditionalResult r1 = await conditionalExecution(condTrue);

    print("Test 2: condition false");
    Bool condFalse = new Bool(false);
    ConditionalResult r2 = await conditionalExecution(condFalse);

    int sum = r1.getValue() + r2.getValue();
    print("Total: " + sum);

    return new Int(sum);
}

function async main(): Promise<Int> {
    Int result = await testConditionalPromises();
    print("Final: " + result);
    return result;
}

main();
