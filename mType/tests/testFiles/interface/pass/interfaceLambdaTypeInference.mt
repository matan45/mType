// Test lambda type inference with interface
// @Script

import * from "../../lib/collections/ArrayList.mt";
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/exceptions/RuntimeException.mt";

interface Comparator<T> {
    function compare(T a, T b): int;
}

class IntComparator implements Comparator<Int> {
    public function compare(Int a, Int b): int {
        if (a.getValue() < b.getValue()) {
            return -1;
        }
        if (a.getValue() > b.getValue()) {
            return 1;
        }
        return 0;
    }

    public function reversed(): Comparator<Int> {
        throw new RuntimeException("reversed not implemented");
    }

    public function thenComparing(Comparator<Int> other): Comparator<Int> {
        throw new RuntimeException("thenComparing not implemented");
    }
}

class Sorter<T> {
    private ArrayList<T> items;

    public constructor(ArrayList<T> items) {
        this.items = items;
    }

    public function sort(Comparator<T> comparator): void {
        int n = this.items.size();
        int i = 0;
        while (i < n - 1) {
            int j = 0;
            while (j < n - i - 1) {
                T a = this.items.get(j);
                T b = this.items.get(j + 1);
                if (comparator.compare(a, b) > 0) {
                    this.items.set(j, b);
                    this.items.set(j + 1, a);
                }
                j = j + 1;
            }
            i = i + 1;
        }
    }
}

ArrayList<Int> numbers = new ArrayList<Int>();
numbers.add(new Int(5));
numbers.add(new Int(2));
numbers.add(new Int(8));
numbers.add(new Int(1));
numbers.add(new Int(9));

Sorter<Int> sorter = new Sorter<Int>(numbers);

// Lambda with inferred types
IntComparator ascendingComparator = new IntComparator();

sorter.sort(ascendingComparator);

print("Sorted:");
int i = 0;
while (i < numbers.size()) {
    print(numbers.get(i).toString());
    i = i + 1;
}
