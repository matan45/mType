// Test: cast a generic class instance to an interface, invoke the interface
// method. Uses a non-parameterized interface because casting to a
// parameterized interface form like `(IHolder<Int>)` currently throws —
// the cast path lacks the MYT-44 substitution that isClassOf has. See
// MYT-367; once fixed, extend or add a sibling fixture using
// `(IDescribable<Int>)` style targets.
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
