import * from "../../lib/collections/List.mt";
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

// Generic library for testing imports with generics
class GenericContainer<T> {
    List<T> items;

    constructor() {
        this.items = new List<T>();
    }

    public function add(T item): void {
        this.items.add(item);
    }

    public function size(): int {
        return this.items.size();
    }

    public function contains(T item): bool {
        // Check if list contains the item
        for (int i = 0; i < this.items.size(); i++) {
            T current = this.items.get(i);
            if (current != null && current.equals(item)) {
                return true;
            }
        }
        return false;
    }

    public function getFirst(): T {
        // Get first item from List (maintains insertion order)
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

function containsIntItem(GenericContainer<Int> container, Int item): bool {
    return container.contains(item);
}

function containsStringItem(GenericContainer<String> container, String item): bool {
    return container.contains(item);
}
