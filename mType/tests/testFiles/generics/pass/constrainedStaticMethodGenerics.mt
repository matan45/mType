// Test: Static methods with generic interface constraints
// Expected: Should compile and run successfully
import * from "../../lib/collections/ArrayList.mt";

interface Comparable<T> {
    function compareTo(T other): int;
}

interface Container<T> {
    function add(T item): void;
    function size(): int;
}

class Item implements Comparable<Item> {
    private int value;

    constructor(int val) {
        this.value = val;
    }

    public function compareTo(Item other): int {
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

class Utilities {
    // Static method with constraint
    public static function <T extends Comparable<T>> findMax(ArrayList<T> items): T? {
        if (items.size() == 0) {
            return null;
        }

        T maxItem = items.get(0);
        for (int i = 1; i < items.size(); i = i + 1) {
            T current = items.get(i);
            if (current.compareTo(maxItem) > 0) {
                maxItem = current;
            }
        }
        return maxItem;
    }

    // Static method with constraint
    public static function <T extends Comparable<T>> findMin(ArrayList<T> items): T? {
        if (items.size() == 0) {
            return null;
        }

        T minItem = items.get(0);
        for (int i = 1; i < items.size(); i = i + 1) {
            T current = items.get(i);
            if (current.compareTo(minItem) < 0) {
                minItem = current;
            }
        }
        return minItem;
    }

    // Multiple generic parameters with constraints
    static function <T extends Comparable<T>, C extends Container<T>> addSorted(C container, T item): void {
        container.add(item);
    }
}

// Test static methods with constraints
ArrayList<Item> items = new ArrayList<Item>();
items.add(new Item(10));
items.add(new Item(5));
items.add(new Item(20));
items.add(new Item(3));

Item? maxItem = Utilities::findMax<Item>(items);
Item? minItem = Utilities::findMin<Item>(items);

if (maxItem != null) {
    print("Max value: " + maxItem.getValue());
}
if (minItem != null) {
    print("Min value: " + minItem.getValue());
}

print("Constrained static method generics test passed!");
