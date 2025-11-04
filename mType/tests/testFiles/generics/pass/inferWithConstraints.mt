import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

// Type inference with bounded constraints
interface Printable {
    function toString(): String;
}

class Item implements Printable {
    String name;

    public constructor(String n) {
        name = n;
    }

    public function toString(): String {
        return name;
    }
}

class Container<T extends Printable> {
    T value;

    public function setValue(T val): void {
        value = val;
    }

    public function print(): void {
        print(value.toString().toString());
    }
}

function <T extends Printable> makeContainer(T val): Container<T> {
    Container<T> c = new Container<T>();
    c.setValue(val);
    return c;
}

function main(): void {
    // Infer T as Item with constraint satisfaction
    Container<Item> container = makeContainer<Item>(new Item(new String("Widget")));
    container.print();
}

main();
