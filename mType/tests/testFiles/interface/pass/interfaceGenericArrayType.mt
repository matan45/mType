// Test array of generic interface type
// @Script

interface Container<T> {
    func add(item: T): void;
    func get(index: Int): T;
    func size(): Int;
}

class SimpleContainer<T> implements Container<T> {
    var items: Array<T>;

    func init() {
        this.items = new Array<T>();
    }

    func add(item: T): void {
        this.items.add(item);
    }

    func get(index: Int): T {
        return this.items.get(index);
    }

    func size(): Int {
        return this.items.size();
    }
}

// Array of generic interface type
var containers = new Array<Container<String>>();

var c1 = new SimpleContainer<String>();
c1.add("Hello");
c1.add("World");

var c2 = new SimpleContainer<String>();
c2.add("Foo");
c2.add("Bar");

containers.add(c1);
containers.add(c2);

// Iterate through containers
var i = 0;
while (i < containers.size()) {
    var container = containers.get(i);
    print("Container " + i.toString() + " size: " + container.size().toString());

    var j = 0;
    while (j < container.size()) {
        print("  " + container.get(j));
        j = j + 1;
    }

    i = i + 1;
}
