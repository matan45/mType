// Test super method access with different access modifiers

class Parent {
    private function secretMethod(): string {
        return "Secret";
    }

    protected function protectedMethod(): string {
        return "Protected from Parent";
    }

    public function publicMethod(): string {
        return "Public from Parent";
    }

    // Parent can call its own private methods
    public function callOwnPrivate(): string {
        return secretMethod();
    }
}

class Child extends Parent {
    constructor(): super() {
    }

    // Override protected method
    protected function protectedMethod(): string {
        // Can call parent's protected method via super
        return "Child: " + super.protectedMethod();
    }

    // Override public method
    public function publicMethod(): string {
        // Can call parent's public method via super
        return "Child: " + super.publicMethod();
    }

    public function testSuperCalls(): void {
        print("Protected: " + protectedMethod());
        print("Public: " + publicMethod());
    }
}

function main(): void {
    print("Testing super method access with access modifiers");

    Child child = new Child();

    // Test calling overridden methods that use super
    child.testSuperCalls();

    // Parent can call its own private method
    print("Parent's private via public: " + child.callOwnPrivate());

    print("Super method access test completed");
}

main();
