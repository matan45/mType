// Test: @Override annotation with wrong parameter count
// Expected: Compile Error - parameter count doesn't match

class Parent {
    void calculate(Int x, Int y) {
        print("Calculating");
    }
}

class Child extends Parent {
    @Override
    void calculate(Int x) {
        print("This should fail - wrong parameter count");
    }
}

// This should not compile
Child child = new Child();
child.calculate(5);
