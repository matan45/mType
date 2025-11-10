// Test await in method chaining

import { Int } from "../../lib/primitives/Int.mt";

print("=== Await in Method Chain Test ===");

class ChainedObject {
    int value;

    public constructor(int v) {
        this.value = v;
    }

    public function getValue(): int {
        return this.value;
    }

    public function increment(): ChainedObject {
        this.value = this.value + 1;
        return this;
    }
}

function async getObject(): Promise<ChainedObject> {
    print("Creating object");
    ChainedObject obj = new ChainedObject(10);
    return obj;
}

function async testMethodChain(): Promise<Int> {
    print("Testing method chain");

    // Await then call method
    int val = (await getObject()).getValue();
    print("Value: " + val);

    // Await then call chained methods
    ChainedObject obj = await getObject();
    ChainedObject chained = obj.increment();
    int finalVal = chained.getValue();
    print("Final value: " + finalVal);

    return new Int(finalVal);
}

function async main(): Promise<Int> {
    Int r = await testMethodChain();
    print("Result: " + r);
    return r;
}

main();
