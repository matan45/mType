// Test: Using a class that doesn't implement the required interface
// Expected: Should fail with error - class doesn't implement required interface

interface Comparable<T> {
    function compareTo(T other): int;
}

class Item {
    private int value;

    constructor(int val) {
        this.value = val;
    }

    function getValue(): int {
        return this.value;
    }
    // Note: Item does NOT implement Comparable
}

class SortedList<T extends Comparable<T>> {
    private List<T> items;

    constructor() {
        this.items = new List<T>();
    }

    function add(T item): void {
        this.items.add(item);
    }
}

// ERROR: Item doesn't implement Comparable<Item>
SortedList<Item> list = new SortedList<Item>();
list.add(new Item(5));
