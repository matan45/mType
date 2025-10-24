// Error: Trying to access private parent field via super

class Parent {
    private int secretValue;
    protected int publicValue;

    constructor() {
        secretValue = 100;
        publicValue = 200;
    }
}

class Child extends Parent {
    constructor(): super() {
    }

    public function tryAccessPrivate(): int {
        // ERROR: Cannot access private field from parent
        return super.secretValue;
    }

    public function accessProtected(): int {
        // OK: Can access protected field
        return super.publicValue;
    }
}

function main(): void {
    Child child = new Child();
    print(child.tryAccessPrivate());
}

main();
