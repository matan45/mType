// Test: Generic class casting
import "../../lib/primitives/Int.mt";
class Container<T> {
    public T value;
    public constructor(T v) { this.value = v; }
    public function getValue(): T { return this.value; }
}

class Box<T> extends Container<T> {
    public constructor(T v): super(v) {  }
}

Box<Int> intBox = new Box<Int>(new Int(42));
Container<Int> container = (Container<Int>)intBox;  // Upcast generic class
print(container.getValue());

// Expected output:
// 42
