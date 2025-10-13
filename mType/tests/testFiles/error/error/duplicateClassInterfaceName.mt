// Error: Duplicate type name - class and interface with same name

class Container {
    int value;

    constructor(int v) {
        this.value = v;
    }
}

// ERROR: Interface with same name as class
interface Container {
    function getValue(): int;
}
