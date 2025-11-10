import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

// Type inference in nested generic structures
class Box<T> {
    T value;

    public function setValue(T val): void {
        value = val;
    }

    public function getValue(): T {
        return value;
    }
}

class Container<E> {
    E element;

    public function setElement(E elem): void {
        element = elem;
    }

    public function getElement(): E {
        return element;
    }
}

function <T> createNestedBox(T val): Container<Box<T>> {
    Box<T> box = new Box<T>();
    box.setValue(val);

    Container<Box<T>> container = new Container<Box<T>>();
    container.setElement(box);
    return container;
}

function main(): void {
    // Infer nested generic type Container<Box<Int>>
    Container<Box<Int>> nested = createNestedBox<Int>(new Int(999));
    Box<Int> innerBox = nested.getElement();
    print("Nested value: " + innerBox.getValue().toString());
}

main();
