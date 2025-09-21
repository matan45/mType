import "../lib/collections/HashSet.mt";
import "../lib/primitives/Int.mt";
import "../lib/primitives/String.mt";

// Generic library for testing imports with generics
class GenericContainer<T> {
    HashSet<T> items;

    constructor() {
        this.items = new HashSet<T>();
    }

    function add(T item): void {
        this.items.add(item);
    }

    function size(): int {
        return this.items.size();
    }

    function contains(T item): bool {
        return this.items.contains(item);
    }

    function getFirst(): T {
        // Simple way to get first item from HashSet
        T[] allItems = this.items.toArray();
        if (allItems.length > 0) {
            return allItems[0];
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
