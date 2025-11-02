// Error: Attempting to override final async method

import { Int } from "../../lib/primitives/Int.mt";

class Base {
    public final function async computeValue(): Promise<Int> {
        return new Int(42);
    }
}

class Derived extends Base {
    // ERROR: Cannot override final method
    public function async computeValue(): Promise<Int> {
        return new Int(100);
    }
}

function async main(): Promise<Int> {
    Derived d = new Derived();
    Int result = await d.computeValue();
    return result;
}

main();
