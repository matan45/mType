// MYT-367: parameterized-interface cast. The cast path must walk
// implementedInterfaces with the receiver's generic bindings
// substituted in ({T -> Int}), mirroring what isClassOf already does
// via MYT-44. This is the canonical repro from the ticket.

import * from "../../lib/primitives/Int.mt";

interface IHolder<T> {
    public function getHeld(): T;
}

class Holder<T> implements IHolder<T> {
    public T held;
    public constructor(T h) { this.held = h; }
    public function getHeld(): T { return this.held; }
}

Holder<Int> holderInt = new Holder<Int>(new Int(123));
IHolder<Int> asInterface = (IHolder<Int>)holderInt;
print(asInterface.getHeld().toString());

// Expected output:
// 123
