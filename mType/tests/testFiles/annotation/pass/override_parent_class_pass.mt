// Test: @Override annotation with parent class method
// Expected: Pass - method correctly overrides parent method

class Parent {
    void greet() {
        print("Hello from Parent");
    }

    Int calculate(Int x, Int y) {
        return x + y;
    }
}

class Child extends Parent {
    @Override
    void greet() {
        print("Hello from Child");
    }

    @Override
    Int calculate(Int x, Int y) {
        return x * y;
    }
}

// Test execution
Child child = new Child();
child.greet();
Int result = child.calculate(5, 3);
print(result);
