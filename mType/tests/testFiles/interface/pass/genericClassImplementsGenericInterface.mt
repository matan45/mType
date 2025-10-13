// Generic class implementing generic interface test
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

interface Container<T> {
    function add(T item): bool;
    function get(int index): T;
    function size(): Int;
}

class GenericList<T> implements Container<T> {
    Int listSize;

    constructor() {
        this.listSize = new Int(0);
    }

    public function add(T item): bool {
        this.listSize = new Int(this.listSize.getValue() + 1);
        print("Added item to generic list");
        return true;
    }

    public function get(int index): T {
        return null; // Simplified implementation
    }

    public function size(): Int {
        return this.listSize;
    }
}

GenericList<String> stringList = new GenericList<String>();
stringList.add(new String("42"));
stringList.add(new String("17"));
print("List size: " + stringList.size().getValue());

print("Generic class implements generic interface successful");