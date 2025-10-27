// Test: @Override annotation with wrong parameter count
// Expected: Compile Error - parameter count doesn't match

class Parent {
    function calculate(int x, int y): void {
        print("Calculating");
    }
}

class Child extends Parent {
    @Override
    function calculate(int x): void {
        print("This should fail - wrong parameter count");
    }
}

// This should not compile
Child child = new Child();
child.calculate(5);
