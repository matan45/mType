// Test: Generic class with interface constraints
// Expected: Should compile and run successfully
import * from "../../lib/collections/List.mt";

interface Comparable<T> {
    function compareTo(T other): int;
}

interface Printable {
    function toString(): string;
}

// Simple class implementing Comparable
class Number implements Comparable<Number> {
    private int value;

    constructor(int val) {
        this.value = val;
    }

    public function compareTo(Number other): int {
        if (this.value < other.value) {
            return -1;
        } else if (this.value > other.value) {
            return 1;
        }
        return 0;
    }

    public function getValue(): int {
        return this.value;
    }
}

// Generic container with constraint
class SortedList<T extends Comparable<T>> {
    private List<T> items;

    constructor() {
        this.items = new List<T>();
    }

    public function add(T item): void {
        this.items.add(item);
    }

    public function sort(): void {
        // Simple bubble sort for testing
        int size = this.items.size();
        for (int i = 0; i < size - 1; i = i + 1) {
            for (int j = 0; j < size - i - 1; j = j + 1) {
                T current = this.items.get(j);
                T next = this.items.get(j + 1);
                if (current.compareTo(next) > 0) {
                    // Swap
                    this.items.set(j, next);
                    this.items.set(j + 1, current);
                }
            }
        }
    }

    public function get(int index): T {
        return this.items.get(index);
    }

    public function size(): int {
        return this.items.size();
    }
}

// Test the constrained generic class
SortedList<Number> numbers = new SortedList<Number>();
numbers.add(new Number(5));
numbers.add(new Number(2));
numbers.add(new Number(8));
numbers.add(new Number(1));

numbers.sort();

print("Sorted values:");
for (int i = 0; i < numbers.size(); i = i + 1) {
    print(numbers.get(i).getValue());
}

print("Constrained class generics test passed!");
