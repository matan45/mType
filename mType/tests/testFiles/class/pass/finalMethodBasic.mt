// Test: Final method basic - Class with final methods that work correctly
// Expected: Pass - final methods can be called but not overridden

class Parent {
    public final function process(): string {
        return "Parent final method";
    }

    public final function calculate(int value): int {
        return value * 2;
    }

    public function regularMethod(): string {
        return "Regular method";
    }
}

class Child extends Parent {
    // Child does NOT override final methods - this is valid
    // Child can add new methods or override non-final methods

    public function regularMethod(): string {
        return "Overridden regular method";
    }

    public function childMethod(): string {
        // Child can call parent's final methods
        return "Child calls: " + super.process();
    }
}

// Test that final methods work correctly
Parent parent = new Parent();
print(parent.process());  // Should print: Parent final method
print(parent.calculate(5));  // Should print: 10
print(parent.regularMethod());  // Should print: Regular method

Child child = new Child();
print(child.process());  // Should print: Parent final method (inherited)
print(child.calculate(7));  // Should print: 14 (inherited)
print(child.regularMethod());  // Should print: Overridden regular method
print(child.childMethod());  // Should print: Child calls: Parent final method
