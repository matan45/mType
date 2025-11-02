// Test protected async methods

import { Int } from "../../lib/primitives/Int.mt";

print("=== Async Protected Method Test ===");

class Base {
    int baseValue;

    public constructor(int value) {
        this.baseValue = value;
    }

    protected function async fetchBaseData(): Promise<Int> {
        print("Fetching base data: " + this.baseValue);
        return new Int(this.baseValue);
    }

    public function async processBase(): Promise<Int> {
        print("Processing in base class");
        Int result = await this.fetchBaseData();
        return result;
    }
}

class Derived extends Base {
    int derivedValue;

    public constructor(int base, int derived) {
        super(base);
        this.derivedValue = derived;
    }

    public function async processDerived(): Promise<Int> {
        print("Processing in derived class");
        Int baseData = await this.fetchBaseData();
        int combined = baseData.getValue() + this.derivedValue;
        print("Combined: " + combined);
        return new Int(combined);
    }
}

function async main(): Promise<Int> {
    Derived obj = new Derived(10, 20);

    Int r1 = await obj.processBase();
    print("Base result: " + r1);

    Int r2 = await obj.processDerived();
    print("Derived result: " + r2);

    return r2;
}

main();
