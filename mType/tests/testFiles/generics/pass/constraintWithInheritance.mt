// Test: Interface constraint with interface inheritance
// Expected: Should compile and run successfully

interface Comparable<T> {
    function compareTo(T other): int;
}

interface Sortable<T> extends Comparable<T> {
    function getKey(): int;
}

class Entry implements Sortable<Entry> {
    private int key;
    private string value;

    constructor(int k, string v) {
        this.key = k;
        this.value = v;
    }

    public function compareTo(Entry other): int {
        if (this.key < other.key) {
            return -1;
        } else if (this.key > other.key) {
            return 1;
        }
        return 0;
    }

    public function getKey(): int {
        return this.key;
    }

    public function getValue(): string {
        return this.value;
    }
}

// Generic class constrained to base interface
class Container<T extends Comparable<T>> {
    private T item;

    constructor(T i) {
        this.item = i;
    }

    public function compare(T other): int {
        return this.item.compareTo(other);
    }
}

// Should work because Entry implements Sortable which extends Comparable
Container<Entry> container = new Container<Entry>(new Entry(1, "first"));
int result = container.compare(new Entry(2, "second"));

print("Comparison result: " + result);
print("Constraint with inheritance test passed!");
