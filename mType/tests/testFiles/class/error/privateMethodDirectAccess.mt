// Test: Direct access to private method from child class
// Expected: Should fail - cannot access private method from parent without super

class Parent {
    private function secretMethod(): string {
        return "Secret from parent";
    }

    protected function protectedMethod(): string {
        return "Protected from parent";
    }

    public function callPrivate(): string {
        // OK: Parent can call its own private method
        return secretMethod();
    }
}

class Child extends Parent {
    constructor(): super() {
    }

    public function tryCallPrivate(): string {
        // ERROR: Cannot access private method from parent
        // (direct call without super keyword)
        return secretMethod();
    }

    public function canCallProtected(): string {
        // OK: Can call protected method
        return protectedMethod();
    }
}

function main(): void {
    Child child = new Child();
    print(child.tryCallPrivate());
}

main();
