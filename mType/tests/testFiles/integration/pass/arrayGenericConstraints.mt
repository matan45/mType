// Arrays + Generics Test 1: Generic arrays with type constraints
@Script

class Comparable<T> {
    fun compareTo(other: T): Int {
        return 0;
    }
}

class Number extends Comparable<Number> {
    field value: Int;

    constructor(v: Int) {
        this.value = v;
    }

    fun compareTo(other: Number): Int {
        if (this.value < other.value) {
            return -1;
        } else if (this.value > other.value) {
            return 1;
        }
        return 0;
    }
}

class SortableArray<T extends Comparable<T>> {
    field items: T[];
    field size: Int;

    constructor(capacity: Int) {
        this.items = T[capacity];
        this.size = 0;
    }

    fun add(item: T): Void {
        this.items[this.size] = item;
        this.size = this.size + 1;
    }

    fun get(index: Int): T {
        return this.items[index];
    }

    fun bubbleSort(): Void {
        let i: Int = 0;
        while (i < this.size - 1) {
            let j: Int = 0;
            while (j < this.size - i - 1) {
                if (this.items[j].compareTo(this.items[j + 1]) > 0) {
                    let temp: T = this.items[j];
                    this.items[j] = this.items[j + 1];
                    this.items[j + 1] = temp;
                }
                j = j + 1;
            }
            i = i + 1;
        }
    }
}

let arr: SortableArray<Number> = SortableArray<Number>(5);
arr.add(Number(42));
arr.add(Number(10));
arr.add(Number(99));
arr.add(Number(5));
arr.add(Number(67));

print("Before sorting:");
let i: Int = 0;
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
