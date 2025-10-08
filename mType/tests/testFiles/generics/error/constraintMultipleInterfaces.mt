// Test: Each generic parameter can only extend one interface
// Expected: Should fail with error - multiple constraints not allowed

interface Comparable<T> {
    function compareTo(T other): int;
}

interface Serializable {
    function serialize(): string;
}

// ERROR: Generic parameter cannot extend multiple interfaces
class Container<T extends Comparable<T> extends Serializable> {
    private T item;

    constructor(T i) {
        this.item = i;
    }
}

Container<string> container = new Container<string>("test");
