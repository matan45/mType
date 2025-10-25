import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

// Test type parameter propagation through inheritance (Generic<T> extends Base<T>)
class BaseContainer<T> {
    protected T value;

    constructor(T val) {
        value = val;
    }

    public function getValue(): T {
        return value;
    }

    public function setValue(T val): void {
        value = val;
    }
}

class ExtendedContainer<T> extends BaseContainer<T> {
    T secondValue;

    constructor(T val1, T val2):super(val1) {
        secondValue = val2;
    }

    public function getSecondValue(): T {
        return secondValue;
    }

    public function setSecondValue(T val): void {
        secondValue = val;
    }

    public function swap(): void {
        T temp = super.value;
        super.value = secondValue;
        secondValue = temp;
    }
}

class SpecializedIntContainer extends BaseContainer<Int> {
    constructor(Int val): super(val) {
    }

    public function increment(): void {
        // Note: This would require Int to support arithmetic,
        // so we'll just demonstrate type specialization
        print("Incrementing value");
    }
}

function main(): void {
    print("Testing type parameter propagation");

    // Test 1: Extended generic container with Int
    ExtendedContainer<Int> intContainer = new ExtendedContainer<Int>(new Int(10), new Int(20));
    print("intContainer first value: " + intContainer.getValue().toString());
    print("intContainer second value: " + intContainer.getSecondValue().toString());

    intContainer.swap();
    print("After swap - first value: " + intContainer.getValue().toString());
    print("After swap - second value: " + intContainer.getSecondValue().toString());

    // Test 2: Extended generic container with String
    ExtendedContainer<String> stringContainer = new ExtendedContainer<String>(
        new String("Hello"),
        new String("World")
    );
    print("stringContainer first value: " + stringContainer.getValue().toString());
    print("stringContainer second value: " + stringContainer.getSecondValue().toString());

    // Test 3: Specialized container (T is fixed to Int)
    SpecializedIntContainer specialized = new SpecializedIntContainer(new Int(42));
    print("Specialized container value: " + specialized.getValue().toString());
    specialized.increment();

    // Test 4: Polymorphic assignment
    BaseContainer<Int> baseRef = intContainer;
    print("Polymorphic access to base value: " + baseRef.getValue().toString());

    print("Type parameter propagation test completed");
}

main();
