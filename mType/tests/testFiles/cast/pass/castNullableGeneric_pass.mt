// Test: Nullable generic type casting
class Container<T> {
    T value;

    Container(T val) {
        this.value = val;
    }

    T getValue() {
        return this.value;
    }
}

class Base {}
class Derived extends Base {}

Container<Base>? nullableContainer = new Container<Base>(new Derived());
Container<Base> nonNullContainer = (Container<Base>)nullableContainer;
print(nonNullContainer != null);

// Test with null value
Container<Base>? nullContainer = null;
Base? result = nullContainer == null ? null : (Base)nullContainer.getValue();
print(result == null);

// Expected output:
// true
// true
