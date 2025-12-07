// Test enhanced for-loop with generic type parameter in generic class context
import * from "../../lib/collections/ArrayList.mt";
import * from "../../lib/primitives/String.mt";

class Container<T> {
    ArrayList<T> items;

    constructor() {
        items = new ArrayList<T>();
    }

    public function add(T item): void {
        items.add(item);
    }

    public function printAll(): void {
        // This is the key test: for (T item : items) where T is a generic type parameter
        for (T item : items) {
            print(item.toString());
        }
    }
}

function main(): void {
    Container<String> container = new Container<String>();
    container.add(new String("Hello"));
    container.add(new String("World"));
    container.printAll();
    print("Generic for-each test passed!");
}
main();
