// Test diamond problem with conflicting methods
// @Throw

interface A {
    func getValue(): String;
}

interface B extends A {
    func getValue(): String;  // Redeclares with same signature - OK
    func doSomething(): void;
}

interface C extends A {
    func getValue(): Int;  // Error: Different return type from A
    func doOtherThing(): void;
}

// Error: Cannot implement both B and C due to conflicting getValue() signatures
class Implementation implements B, C {
    func getValue(): String {
        return "test";
    }

    func getValue(): Int {
        return 42;
    }

    func doSomething(): void {
        print("B method");
    }

    func doOtherThing(): void {
        print("C method");
    }
}

var impl = new Implementation();
print(impl.getValue());
