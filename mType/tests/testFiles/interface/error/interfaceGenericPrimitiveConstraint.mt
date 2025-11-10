// Test generic with primitive constraint (should error)
// @Throw

interface Container<T extends Int> {  // Error: Cannot use primitive type as constraint
    func add(item: T): void;
    func get(index: Int): T;
}

class IntContainer implements Container<Int> {
    var items: Array<Int>;

    func init() {
        this.items = new Array<Int>();
    }

    func add(item: Int): void {
        this.items.add(item);
    }

    func get(index: Int): Int {
        return this.items.get(index);
    }
}

var container = new IntContainer();
container.add(42);
