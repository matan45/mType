// Arrays + Generics Test 1: Generic arrays with type constraints
@Script

class Comparable<T> {
    public function compareTo(other: T): Int {
        return 0;
    }
}

class Number extends Comparable<Number> {
    Int value;

    constructor(v: Int) {
        this.value = v;
    }

    public function compareTo(other: Number): Int {
        if (this.value < other.value) {
            return -1;
        } else if (this.value > other.value) {
            return 1;
        }
        return 0;
    }
}

class SortableArray<T extends Comparable<T>> {
    T[] items;
    Int size;

    constructor(capacity: Int) {
        this.items = T[capacity];
        this.size = 0;
    }

    public function add(item: T): void {
        this.items[this.size] = item;
        this.size = this.size + 1;
    }

    public function get(index: Int): T {
        return this.items[index];
    }

    public function bubbleSort(): void {
        Int i = 0;
        while (i < this.size - 1) {
            Int j = 0;
            while (j < this.size - i - 1) {
                if (this.items[j].compareTo(this.items[j + 1]) > 0) {
                    T temp = this.items[j];
                    this.items[j] = this.items[j + 1];
                    this.items[j + 1] = temp;
                }
                j = j + 1;
            }
            i = i + 1;
        }
    }
}

SortableArray<Number> arr = SortableArray<Number>(5);
arr.add(Number(42));
arr.add(Number(10));
arr.add(Number(99));
arr.add(Number(5));
arr.add(Number(67));

print("Before sorting:");
Int i = 0;
while (i < 5) {
    print(arr.get(i).value);
    i = i + 1;
}

arr.bubbleSort();

print("After sorting:");
i = 0;
while (i < 5) {
    print(arr.get(i).value);
    i = i + 1;
}
