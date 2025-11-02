import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

// Return type inference for generic methods
class Box<T> {
    T item;

    public function setItem(T i): void {
        item = i;
    }

    public function getItem(): T {
        return item;
    }
}

function <T> createBox(T value): Box<T> {
    Box<T> box = new Box<T>();
    box.setItem(value);
    return box;
}

function main(): void {
    // Infer return type from generic method
    Box<Int> intBox = createBox(new Int(123));
    print("Box contains: " + intBox.getItem());

    Box<String> strBox = createBox(new String("text"));
    print("Box contains: " + strBox.getItem());
}

main();
