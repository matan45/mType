// Test: Static method constraint violation
// Expected: Should fail - class doesn't implement required interface

interface Comparable<T> {
    function compareTo(T other): int;
}

class SimpleClass {
    private int value;

    constructor(int val) {
        this.value = val;
    }
    // Does NOT implement Comparable
}

class Utilities {
    static function <T extends Comparable<T>> findMax(List<T> items): T {
        return items.get(0);
    }
}

// ERROR: SimpleClass doesn't implement Comparable<SimpleClass>
List<SimpleClass> items = new List<SimpleClass>();
items.add(new SimpleClass(10));
SimpleClass max = Utilities::<SimpleClass>findMax(items);
