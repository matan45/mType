// Test: isClassOf with generic class (simplified)
class Box<T> {
    T value;
    Box(T v) { this.value = v; }
}

Box<int> box = new Box<int>(42);
print(box isClassOf Box); // true

// Expected output:
// true
