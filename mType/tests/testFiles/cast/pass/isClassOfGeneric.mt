// Test: isClassOf with generic class (simplified)
import * from "../../lib/primitives/Int.mt";
class Box<T> {
    T value;
    constructor(T v) { this.value = v; }
}

Box<Int> box = new Box<Int>(new Int(42));
print(box isClassOf Box); // true

// Expected output:
// true
