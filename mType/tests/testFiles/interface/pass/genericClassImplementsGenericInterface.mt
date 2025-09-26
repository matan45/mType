// Generic class implementing generic interface test
import "../../lib/primitives/Int.mt";
import "../../lib/primitives/String.mt";

interface Container<T> {
    function add(T item): bool;
    function get(int index): T;
    function size(): Int;
}

class GenericList<T> implements Container<Int> {
    Int listSize;

    constructor() {
        this.listSize = new Int(0);
    }

    function add(Int item): bool {
        this.listSize = new Int(this.listSize.getValue() + 1);
        print("Added item to generic list");
        return true;
    }

    function get(int index): Int {
        return null; // Simplified implementation
    }

    function size(): Int {
        return this.listSize;
    }
}

GenericList<String> intList = new GenericList<String>();
intList.add(new String("42"));
intList.add(new String("17"));
print("List size: " + intList.size().getValue());

print("Generic class implements generic interface successful");