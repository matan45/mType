// MYT-367 negative regression: parameterized-interface cast with a
// mismatched type argument MUST still throw. The fix substitutes the
// receiver's bindings before comparison — it must accept matching
// bindings without becoming over-permissive (e.g. comparing only base
// names and silently passing). Holder<Int> -> IHolder<String> would
// match if substitution were skipped entirely.

import * from "../../lib/primitives/Int.mt";

interface IHolder<T> {
    public function getHeld(): T;
}

class Holder<T> implements IHolder<T> {
    public T held;
    public constructor(T h) { this.held = h; }
    public function getHeld(): T { return this.held; }
}

Holder<Int> holderInt = new Holder<Int>(new Int(5));
IHolder<string> wrong = (IHolder<string>)holderInt;
print(wrong.getHeld());
