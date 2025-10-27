// Test: @Override annotation with wrong method signature
// Expected: Compile Error - parameter types don't match

class Parent {
    function process(int value): void {
        print("Processing int");
    }
}

class Child extends Parent {
    @Override
    function process(string value): void {
        print("This should fail - wrong parameter type");
    }
}

// This should not compile
Child child = new Child();
child.process("test");
