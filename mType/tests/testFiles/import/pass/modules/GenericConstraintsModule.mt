interface Comparable<T> {
    fun compareTo(other: T): Int;
}

class SortedBox<T extends Comparable<T>> {
    var items: Array<T>;
    var count: Int;

    constructor() {
        this.items = Array<T>(10);
        this.count = 0;
    }

    fun add(item: T): Void {
        this.items[this.count] = item;
        this.count = this.count + 1;
    }

    fun getCount(): Int {
        return this.count;
    }
}
