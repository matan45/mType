// Test method conflict across inheritance chain
// @Throw

interface Base {
    func getValue(): String;
}

interface Derived extends Base {
    func getValue(): Int;  // Error: Conflicting return type with inherited method
}

class Implementation implements Derived {
    // Cannot implement both - conflicting signatures
    func getValue(): String {
        return "test";
    }

    func getValue(): Int {
        return 42;
    }
}

var impl = new Implementation();
print(impl.getValue());
