// MYT-367 regression: parameterized-interface cast with NESTED generic
// arguments (IHolder<Wrapper<Int>>). Exercises the recursive branch of
// substituteTypeExpression so a substitution bug at depth > 1 won't be
// hidden by the flat-arg fixtures.

import * from "../../lib/primitives/Int.mt";

interface IHolder<T> {
    public function get(): T;
}

class Wrapper<T> {
    public T value;
    public constructor(T v) { this.value = v; }
}

class Holder<T> implements IHolder<T> {
    public T held;
    public constructor(T h) { this.held = h; }
    public function get(): T { return this.held; }
}

Holder<Wrapper<Int>> h = new Holder<Wrapper<Int>>(new Wrapper<Int>(new Int(42)));
IHolder<Wrapper<Int>> asIface = (IHolder<Wrapper<Int>>)h;
print(asIface.get().value.toString());

// Expected output:
// 42
