// Test type erasure edge cases with generics
// @Script

interface Container<T> {
    func add(item: T): void;
    func size(): Int;
}

class GenericContainer<T> implements Container<T> {
    var count: Int;

    func init() {
        this.count = 0;
    }

    func add(item: T): void {
        this.count = this.count + 1;
        // After type erasure, we can't inspect the actual type of T at runtime
    }

    func size(): Int {
        return this.count;
    }
}

// Different instantiations of the same generic class
var stringContainer = new GenericContainer<String>();
var intContainer = new GenericContainer<Int>();

stringContainer.add("Hello");
stringContainer.add("World");
intContainer.add(42);
intContainer.add(100);

print(stringContainer.size());  // Should print 2
print(intContainer.size());     // Should print 2

// Type erasure means both containers have same runtime type structure
// but different compile-time type parameters
var c1: Container<String> = stringContainer;
var c2: Container<Int> = intContainer;

print(c1.size());  // Should print 2
print(c2.size());  // Should print 2
