import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

// Test generic method type parameter inference
class Container<T> {
    T value;

    public function setValue(T newValue): void {
        value = newValue;
    }

    public function getValue(): T {
        return value;
    }
}

// Generic method that should infer type from arguments
function <T> identity(T item): T {
    return item;
}

// Generic method with multiple parameters
function <T> select(T first, T second, bool useFirst): T {
    if (useFirst) {
        return first;
    }
    return second;
}

// Generic method that wraps value in container
function <T> wrap(T value): Container<T> {
    Container<T> container = new Container<T>();
    container.setValue(value);
    return container;
}

function main(): void {
    print("Testing generic method type inference");

    // Test identity with Int
    Int num = new Int(42);
    Int result1 = identity<Int>(num);
    print("identity<Int>(42): " + result1);

    // Test identity with String
    String str = new String("Hello");
    String result2 = identity<String>(str);
    print("identity<String>(Hello): " + result2);

    // Test select with Int
    Int a = new Int(10);
    Int b = new Int(20);
    Int result3 = select<Int>(a, b, true);
    print("select<Int>(10, 20, true): " + result3);

    Int result4 = select<Int>(a, b, false);
    print("select<Int>(10, 20, false): " + result4);

    // Test wrap with Int
    Container<Int> wrapped = wrap<Int>(new Int(99));
    print("wrap<Int>(99).getValue(): " + wrapped.getValue());

    print("Generic method inference test completed");
}

main();
