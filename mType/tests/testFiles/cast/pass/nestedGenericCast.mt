// Test: nested generic types; upcast Box<Container<Int>> to
// Container<Container<Int>> and access the nested inner value.
import * from "../../lib/primitives/Int.mt";

class Container<T> {
    public T value;
    public constructor(T v) { this.value = v; }
    public function getValue(): T { return this.value; }
}

class Box<T> extends Container<T> {
    public constructor(T v) : super(v) {}
}

Container<Int> inner = new Container<Int>(new Int(7));
Box<Container<Int>> outer = new Box<Container<Int>>(inner);

Container<Container<Int>> upcasted = (Container<Container<Int>>)outer;
Container<Int> retrieved = upcasted.getValue();
print(retrieved.getValue().toString());

// Expected output:
// 7
