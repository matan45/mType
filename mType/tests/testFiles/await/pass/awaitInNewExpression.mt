// Test await in constructor arguments

import { Int } from "../../lib/primitives/Int.mt";

print("=== Await in New Expression Test ===");

class Container {
    int value;

    public constructor(int v) {
        print("Container constructor with value: " + v);
        this.value = v;
    }

    public function getValue(): int {
        return this.value;
    }
}

function async getInitialValue(): Promise<Int> {
    print("Getting initial value");
    return new Int(42);
}

function async testNewExpression(): Promise<Container> {
    print("Creating object with awaited value");

    // Await in constructor argument
    Container obj = new Container((await getInitialValue()).getValue());
    print("Object created with value: " + obj.getValue());

    return obj;
}

function async main(): Promise<Int> {
    Container c = await testNewExpression();
    print("Final value: " + c.getValue());
    return new Int(c.getValue());
}

main();
