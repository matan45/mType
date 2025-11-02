// Test class implements Container<T> where class is also generic
// @Script

interface Container<T> {
    func add(item: T): void;
    func get(index: Int): T;
}

// Generic class that partially substitutes the interface generic
class Pair<T, U> implements Container<T> {
    var first: T;
    var second: T;
    var extra: U;

    func init(extra: U) {
        this.extra = extra;
    }

    func add(item: T): void {
        if (this.first == null) {
            this.first = item;
        } else {
            this.second = item;
        }
    }

    func get(index: Int): T {
        if (index == 0) {
            return this.first;
        }
        return this.second;
    }

    func getExtra(): U {
        return this.extra;
    }
}

var pair = new Pair<String, Int>(42);
pair.add("Hello");
pair.add("World");

print(pair.get(0));       // Should print "Hello"
print(pair.get(1));       // Should print "World"
print(pair.getExtra());   // Should print 42
