// Test reassigning promise variables and awaiting different promises

import { Int } from "../../lib/primitives/Int.mt";

print("=== Promise Reassignment Test ===");

class Value {
    int num;

    public constructor(int n) {
        this.num = n;
    }

    public function getNum(): int {
        return this.num;
    }
}

function async makeValue(int n): Promise<Value> {
    print("Making value: " + n);
    Value v = new Value(n);
    return v;
}

function async testReassignment(): Promise<Int> {
    print("Creating first promise");
    Promise<Value> p = makeValue(10);

    Value v1 = await p;
    print("First value: " + v1.getNum());

    print("Reassigning promise variable");
    p = makeValue(20);

    Value v2 = await p;
    print("Second value: " + v2.getNum());

    print("Reassigning again");
    p = makeValue(30);

    Value v3 = await p;
    print("Third value: " + v3.getNum());

    int sum = v1.getNum() + v2.getNum() + v3.getNum();
    print("Sum: " + sum);

    return new Int(sum);
}

function async main(): Promise<Int> {
    Int result = await testReassignment();
    print("Result: " + result);
    return result;
}

main();
