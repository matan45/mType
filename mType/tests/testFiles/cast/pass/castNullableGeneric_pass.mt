// Test: Nullable generic type casting
class Container<T> {
    T value;

    constructor(T val) {
        this.value = val;
    }

    public function getValue(): T {
        return this.value;
    }
}

class Base {}
class Derived extends Base {}

Container<Base> nullableContainer = new Container<Base>(new Derived());
Container<Base> nonNullContainer = nullableContainer;
print(nonNullContainer != null);

// Test with null value
Container<Base>? nullContainer = null;
Base? result = null;
if (nullContainer != null) {
    result = (Base)nullContainer.getValue();
}
print(result == null);

// Expected output:
// true
// true
