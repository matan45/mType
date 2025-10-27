// Test: @Override annotation in class with no parent
// Expected: Compile Error - no parent class or interface

class Standalone {
    @Override
    function someMethod(): void {
        print("This should fail - no parent to override from");
    }
}

// This should not compile
Standalone obj = new Standalone();
obj.someMethod();
