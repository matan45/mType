// Test: Generic type mismatch during cast should fail
import "../../lib/primitives/Int.mt";
import "../../lib/primitives/String.mt";

class Box<T> {
    T value;
    constructor(T v) { this.value = v; }
}

Box<Int> intBox = new Box<Int>(42);
Box<String> stringBox = (Box<String>)intBox;  // Error: incompatible generic types
print(stringBox.value);
