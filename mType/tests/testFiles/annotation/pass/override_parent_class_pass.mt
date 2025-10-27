// Test: @Override annotation with parent class method
// Expected: Pass - method correctly overrides parent method

class Parent {
    public function greet(): void {
        print("Hello from Parent");
    }

    public function calculate(int x, int y): int {
        return x + y;
    }
}

class Child extends Parent {
    @Override
    public function greet(): void {
        print("Hello from Child");
    }

    @Override
    public function calculate(int x, int y): int {
        return x * y;
    }
}

// Test execution
Child child = new Child();
child.greet();
int result = child.calculate(5, 3);
print(result);
