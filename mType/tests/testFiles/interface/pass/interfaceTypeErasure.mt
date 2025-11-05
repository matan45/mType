// Test type erasure edge cases with generics
// @Script

import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

interface Container<T> {
    function add(T item): void;
    function size(): int;
}

class GenericContainer<T> implements Container<T> {
    private int count;

    public constructor() {
        this.count = 0;
    }

    public function add(T item): void {
        this.count = this.count + 1;
        // After type erasure, we can't inspect the actual type of T at runtime
    }

    public function size(): int {
        return this.count;
    }
}

// Different instantiations of the same generic class
GenericContainer<String> stringContainer = new GenericContainer<String>();
GenericContainer<Int> intContainer = new GenericContainer<Int>();

stringContainer.add(new String("Hello"));
stringContainer.add(new String("World"));
intContainer.add(new Int(42));
intContainer.add(new Int(100));

print(stringContainer.size());  // Should print 2
print(intContainer.size());     // Should print 2

// Type erasure means both containers have same runtime type structure
// but different compile-time type parameters
Container<String> c1 = stringContainer;
Container<Int> c2 = intContainer;

print(c1.size());  // Should print 2
print(c2.size());  // Should print 2
