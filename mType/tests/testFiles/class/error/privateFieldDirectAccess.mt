// Test: Direct access to private field from child class
// Expected: Should fail - cannot access private field from parent

class Parent {
    private int secretValue;
    protected int protectedValue;
    public int publicValue;

    constructor() {
        this.secretValue = 42;
        this.protectedValue = 100;
        this.publicValue = 200;
    }

    public function getSecret(): int {
        // OK: Parent can access its own private field
        return this.secretValue;
    }
}

class Child extends Parent {
    constructor(): super() {
    }

    public function tryAccessPrivate(): int {
        // ERROR: Cannot access private field from parent
        return this.secretValue;
    }

    public function tryModifyPrivate(): void {
        // ERROR: Cannot modify private field from parent
        this.secretValue = 999;
    }

    public function canAccessProtected(): int {
        // OK: Can access protected field
        return this.protectedValue;
    }

    public function canAccessPublic(): int {
        // OK: Can access public field
        return this.publicValue;
    }
}

function main(): void {
    Child child = new Child();
    print("Private: " + child.tryAccessPrivate());
}

main();
