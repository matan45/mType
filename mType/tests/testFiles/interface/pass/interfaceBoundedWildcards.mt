// Test bounded generic types beyond simple extends
// @Script

import * from "../../lib/collections/ArrayList.mt";

interface Comparable<T> {
    function compareTo(T other): int;
}

class Number implements Comparable<Number> {
    public int value;

    public constructor(int value) {
        this.value = value;
    }

    public function compareTo(Number other): int {
        if (this.value < other.value) {
            return -1;
        }
        if (this.value > other.value) {
            return 1;
        }
        return 0;
    }
}

interface Container<T extends Comparable<T>> {
    function add(T item): void;
    function findMax(): T;
}

class NumberContainer implements Container<Number> {
    private ArrayList<Number> items;

    public constructor() {
        this.items = new ArrayList<Number>();
    }

    public function add(Number item): void {
        this.items.add(item);
    }

    public function findMax(): Number {
        if (this.items.size() == 0) {
            return new Number(0);
        }

        Number max = this.items.get(0);
        for (int i = 1; i < this.items.size(); i++) {
            Number current = this.items.get(i);
            if (current.compareTo(max) > 0) {
                max = current;
            }
        }
        return max;
    }
}

NumberContainer container = new NumberContainer();
container.add(new Number(5));
container.add(new Number(10));
container.add(new Number(3));

Number max = container.findMax();
print(max.value);  // Should print 10
