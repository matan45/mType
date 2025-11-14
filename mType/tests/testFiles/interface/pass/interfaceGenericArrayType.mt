// Test array of generic interface type
// @Script

import * from "../../lib/collections/ArrayList.mt";
import * from "../../lib/primitives/String.mt";

interface Container<T> {
    function add(T item): void;
    function get(int index): T;
    function size(): int;
}

class SimpleContainer<T> implements Container<T> {
    private ArrayList<T> items;

    public constructor() {
        this.items = new ArrayList<T>();
    }

    public function add(T item): void {
        this.items.add(item);
    }

    public function get(int index): T {
        return this.items.get(index);
    }

    public function size(): int {
        return this.items.size();
    }
}

// Array of generic interface type
ArrayList<Container<String>> containers = new ArrayList<Container<String>>();

Container<String> c1 = new SimpleContainer<String>();
c1.add(new String("Hello"));
c1.add(new String("World"));

Container<String> c2 = new SimpleContainer<String>();
c2.add(new String("Foo"));
c2.add(new String("Bar"));

containers.add(c1);
containers.add(c2);

// Iterate through containers
for (int i = 0; i < containers.size(); i++) {
    Container<String> container = containers.get(i);
    print("Container " + i + " size: " + container.size());

    for (int j = 0; j < container.size(); j++) {
        print("  " + container.get(j));
    }
}
