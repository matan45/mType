// Test: cast a generic class instance to a non-parameterized interface,
// invoke the interface method. The parameterized-interface form
// (IHolder<Int>) is covered by genericInterfaceCastParameterized.mt
// (MYT-367 — fixed).
import * from "../../lib/primitives/Int.mt";

interface IDescribable {
    public function describe(): void;
}

class Holder<T> implements IDescribable {
    public T held;
    public constructor(T h) { this.held = h; }
    public function describe(): void { print("Holder holds value"); }
}

Holder<Int> impl = new Holder<Int>(new Int(123));
IDescribable asInterface = (IDescribable)impl;
asInterface.describe();
print(impl.held.toString());

// Expected output:
// Holder holds value
// 123
