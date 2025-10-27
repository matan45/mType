// Test: @Override annotation without parent method
// Expected: Compile Error - no method to override

class Parent {
    function existingMethod(): void {
        print("Existing method");
    }
}

class Child extends Parent {
    @Override
    public function nonExistentMethod(): void {
        print("This should fail");
    }
}

// This should not compile
Child child = new Child();
child.nonExistentMethod();
