// MYT-367 regression: cast to a parameterized ANCESTOR interface (not
// the directly-implemented one). Exercises checkInterfaceHierarchyParam
// recursion + rebindForInterface across an interface-extends-interface
// chain. Uses divergent parameter letters (A class, B middle iface, E
// root iface) to force rebind at every level — direct-binding shortcuts
// would mask a recursion bug.

import * from "../../lib/primitives/Int.mt";

interface IBase<E> {
    public function get(): E;
}

interface IExtended<B> extends IBase<B> {
    public function tag(): void;
}

class Box<A> implements IExtended<A> {
    public A held;
    public constructor(A v) { this.held = v; }
    public function get(): A { return this.held; }
    public function tag(): void { print("tagged"); }
}

Box<Int> b = new Box<Int>(new Int(99));
IBase<Int> asBase = (IBase<Int>)b;
print(asBase.get().toString());

// Expected output:
// 99
