// Test: generic class implementing a generic interface; cast the implementer
// to the interface type and invoke the interface method.
import * from "../../lib/primitives/Int.mt";

interface IHolder<T> {
    public function getHeld(): T;
}

class Holder<T> implements IHolder<T> {
    public T held;
    public constructor(T h) { this.held = h; }
    public function getHeld(): T { return this.held; }
}

Holder<Int> impl = new Holder<Int>(new Int(123));
IHolder<Int> asInterface = (IHolder<Int>)impl;
print(asInterface.getHeld().toString());

// Expected output:
// 123
