// Test class implements Container<T> where class is also generic
// @Script

import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

interface Container<T> {
    function add(T item): void;
    function get(int index): T;
}

// Generic class that partially substitutes the interface generic
class Pair<T, U> implements Container<T> {
    private T first;
    private T second;
    private U extra;

    public constructor(U extra) {
        this.extra = extra;
    }

    public function add(T item): void {
        if (this.first == null) {
            this.first = item;
        } else {
            this.second = item;
        }
    }

    public function get(int index): T {
        if (index == 0) {
            return this.first;
        }
        return this.second;
    }

    public function getExtra(): U {
        return this.extra;
    }
}

Pair<String, Int> pair = new Pair<String, Int>(new Int(42));
pair.add(new String("Hello"));
pair.add(new String("World"));

print(pair.get(0));       // Should print "Hello"
print(pair.get(1));       // Should print "World"
print(pair.getExtra());   // Should print 42
