// Test interface with methods having their own generic types
// @Script

interface Container {
    func add<T>(item: T): void;
    func get<T>(index: Int): T;
    func transform<T, R>(item: T, mapper: func(T): R): R;
}

class SimpleContainer implements Container {
    var items: Array<Any>;

    func init() {
        this.items = new Array<Any>();
    }

    func add<T>(item: T): void {
        this.items.add(item);
    }

    func get<T>(index: Int): T {
        return this.items.get(index);
    }

    func transform<T, R>(item: T, mapper: func(T): R): R {
        return mapper(item);
    }
}

var container = new SimpleContainer();
container.add<String>("Hello");
container.add<Int>(42);

var str: String = container.get<String>(0);
var num: Int = container.get<Int>(1);

var doubled = container.transform<Int, Int>(5, func(x: Int): Int { return x * 2; });

print(str);
print(num);
print(doubled);  // Should print 10
