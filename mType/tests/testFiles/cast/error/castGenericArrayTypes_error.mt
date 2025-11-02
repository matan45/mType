// Error: Generic array type mismatch
// Cannot cast between incompatible generic array types

class Container<T> {
    T value;

    constructor(T val) {
        value = val;
    }

    function getValue(): T {
        return value;
    }
}

class Box<T> {
    T item;

    constructor(T i) {
        item = i;
    }
}

@Script
function testGenericArrayTypeMismatch(): void {
    // Create array of Container<int>
    Container<int>[] intContainers = new Container<int>[3];
    intContainers[0] = new Container<int>(10);
    intContainers[1] = new Container<int>(20);
    intContainers[2] = new Container<int>(30);

    // Error: Cannot cast Container<int>[] to Container<string>[]
    // Generic type parameters are incompatible
    Container<string>[] stringContainers = (Container<string>[])intContainers;
}
