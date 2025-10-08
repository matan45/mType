// Error test: Cannot extend a final generic class

final class FinalContainer<T> {
    T item;

    constructor(T i) {
        item = i;
    }

    function getItem(): T {
        return item;
    }
}

// This should fail - attempting to extend a final generic class
class ExtendedContainer<T> extends FinalContainer<T> {
    constructor(T i) {
        super(i);
    }
}
