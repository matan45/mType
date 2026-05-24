// Test: generic class extending a generic class; upcast Box<T> to Container<T>
// and verify polymorphic method dispatch and field access through the cast.
import * from "../../lib/primitives/Int.mt";

class Container<T> {
    public T value;
    public constructor(T v) { this.value = v; }
    public function describe(): void { print("Container holds value"); }
}

class Box<T> extends Container<T> {
    public constructor(T v) : super(v) {}
    public function describe(): void { print("Box wraps value"); }
}

Box<Int> b = new Box<Int>(new Int(99));
Container<Int> c = (Container<Int>)b;
c.describe();
print(c.value.toString());

// Expected output:
// Box wraps value
// 99
