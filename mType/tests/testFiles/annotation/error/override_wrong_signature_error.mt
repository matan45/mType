// Test: @Override annotation with wrong method signature
// Expected: Compile Error - parameter types don't match

class Parent {
    void process(Int value) {
        print("Processing int");
    }
}

class Child extends Parent {
    @Override
    void process(String value) {
        print("This should fail - wrong parameter type");
    }
}

// This should not compile
Child child = new Child();
child.process("test");
