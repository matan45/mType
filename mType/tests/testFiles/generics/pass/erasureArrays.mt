import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

// Type erasure with generic arrays
class ArrayWrapper<T> {
    T[] items;
    Int capacity;

    public function ArrayWrapper(Int cap) {
        capacity = cap;
        items = new T[cap.value];
    }

    public function set(Int index, T value): void {
        items[index.value] = value;
    }

    public function get(Int index): T {
        return items[index.value];
    }
}

function main(): void {
    ArrayWrapper<String> stringArray = new ArrayWrapper<String>(new Int(3));
    stringArray.set(new Int(0), new String("First"));
    stringArray.set(new Int(1), new String("Second"));

    print(stringArray.get(new Int(0)));
    print(stringArray.get(new Int(1)));

    ArrayWrapper<Int> intArray = new ArrayWrapper<Int>(new Int(2));
    intArray.set(new Int(0), new Int(100));
    intArray.set(new Int(1), new Int(200));

    print(intArray.get(new Int(0)));
    print(intArray.get(new Int(1)));
}

main();
