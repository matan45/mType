interface Comparable<T> {
    function compareTo(T other): int;
}

class SortedBox<T extends Comparable<T>> {
    T[] items;
    int count;

    constructor() {
        this.items = new T[10];
        this.count = 0;
    }

    public function add(T item ): void {
        this.items[this.count] = item;
        this.count = this.count + 1;
    }

    public function getCount(): int {
        return this.count;
    }
}
