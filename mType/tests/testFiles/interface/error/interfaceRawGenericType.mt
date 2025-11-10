// Test using raw generic interface (should error or warn)
// @Throw

interface Container<T> {
    func add(item: T): void;
    func get(index: Int): T;
}

class StringContainer implements Container<String> {
    var items: Array<String>;

    func init() {
        this.items = new Array<String>();
    }

    func add(item: String): void {
        this.items.add(item);
    }

    func get(index: Int): String {
        return this.items.get(index);
    }
}

// Error: Using raw type Container without type parameter
var container: Container = new StringContainer();
container.add("Hello");  // Type safety lost
