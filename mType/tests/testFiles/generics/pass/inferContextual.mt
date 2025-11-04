import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

// Contextual type inference
class Container<T> {
    T[] items;
    Int count;

    public constructor() {
        items = new T[5];
        count = new Int(0);
    }

    public function add(T item): void {
        items[count.value] = item;
        count = new Int(count.value + 1);
    }

    public function get(Int index): T {
        return items[index.value];
    }
}

function <T> processContainer(Container<T> container, T item): void {
    container.add(item);
}

function main(): void {
    Container<String> strContainer = new Container<String>();
    // Contextual inference from container type
    processContainer<String>(strContainer, new String("hello"));
    processContainer<String>(strContainer, new String("world"));

    print(strContainer.get(new Int(0)).toString());
    print(strContainer.get(new Int(1)).toString());
}

main();
