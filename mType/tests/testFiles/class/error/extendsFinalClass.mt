// Error test: Cannot extend a final class
// This should throw an InheritanceException

final class FinalBase {
    int value;

    constructor(int v) {
        value = v;
    }

    function getValue(): int {
        return value;
    }
}

// This should fail - attempting to extend a final class
class Derived extends FinalBase {
    constructor(int v) {
        super(v);
    }
}
