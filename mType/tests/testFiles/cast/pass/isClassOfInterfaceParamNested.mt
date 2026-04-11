// MYT-44: nested parameterized type arguments substitute correctly.
// `class WrapperHolder<E> implements MyHolder<Wrapper<E>>` — substituting
// with {E: Int} must produce "MyHolder<Wrapper<Int>>" (with ", " spacing
// for multi-arg cases, though this test has only one arg).
// Exercises both recursive substitution AND the depth-aware splitter
// that MUST NOT split on commas inside nested angle brackets.

import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

interface MyHolder<T> {
    function retrieve(): T;
}

class Wrapper<V> {
    V inner;
    public constructor(V v) { this.inner = v; }
}

class WrapperHolder<E> implements MyHolder<Wrapper<E>> {
    Wrapper<E> stored;
    public constructor(Wrapper<E> w) { this.stored = w; }
    public function retrieve(): Wrapper<E> { return this.stored; }
}

WrapperHolder<Int> wh = new WrapperHolder<Int>(new Wrapper<Int>(new Int(42)));

print(wh isClassOf MyHolder<Wrapper<Int>>);    // true
print(wh isClassOf MyHolder<Wrapper<String>>); // false — nested mismatch
print(wh isClassOf MyHolder<Int>);             // false — must not unwrap
print(wh isClassOf MyHolder);                  // true — raw fallback
