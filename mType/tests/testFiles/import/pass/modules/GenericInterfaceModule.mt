interface Collection<T> {
    fun add(item: T): Void;
    fun size(): Int;
    fun get(index: Int): T;
}

class SimpleCollection<T> implements Collection<T> {
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

    fun size(): Int {
        return this.count;
    }

    fun get(index: Int): T {
        return this.items[index];
    }
}
