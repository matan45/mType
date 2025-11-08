public interface Collection<T> {
    function add(T item): void;
    function size(): int;
    function get(int index): T;
}

public class SimpleCollection<T> implements Collection<T> {
    T[] items;
    int count;

    constructor() {
        this.items = new T[10];
        this.count = 0;
    }

    public function add(T item): void {
        this.items[this.count] = item;
        this.count = this.count + 1;
    }

    public function size(): int {
        return this.count;
    }

    public function get(int index): T {
        return this.items[index];
    }
}
