import * from "../../lib/collections/ArrayList.mt";
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/Bool.mt";
import * from "../../lib/primitives/String.mt";

// Generic library for testing imports with generics
class GenericContainer<T> {
    ArrayList<T> items;

    constructor() {
        this.items = new ArrayList<T>();
    }

    public function add(T item): void {
        this.items.add(item);
    }

    public function size(): Int {
        return this.items.size();
    }

    public function contains(T item): Bool {
        // Check if ArrayList contains the item
        for (int i = 0; i < this.items.size(); i++) {
            T current = this.items.get(i);
            if (current != null && current.equals(item)) {
                return new Bool(true);
            }
        }
        return new Bool(false);
    }

    public function getFirst(): T? {
        // Get first item from ArrayList (maintains insertion order)
        if (this.items.size() > 0) {
            return this.items.get(0);
        }
        return null;
    }
}

// Utility functions for specific types since generic functions aren't supported
function createIntContainer(): GenericContainer<Int> {
    return new GenericContainer<Int>();
}

function createStringContainer(): GenericContainer<String> {
    return new GenericContainer<String>();
}

function containsIntItem(GenericContainer<Int> container, Int item): Bool {
    return container.contains(item);
}

function containsStringItem(GenericContainer<String> container, String item): Bool {
    return container.contains(item);
}
