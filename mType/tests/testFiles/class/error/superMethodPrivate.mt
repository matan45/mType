// Error: Trying to call private parent method via super

class Parent {
    private function secretMethod(): string {
        return "Secret";
    }

    protected function protectedMethod(): string {
        return "Protected";
    }

    public function publicMethod(): string {
        return "Public";
    }
}

class Child extends Parent {
    constructor(): super() {
    }

    public function tryCallPrivate(): string {
        // ERROR: Cannot call private method from parent
        return super.secretMethod();
    }

    public function callProtected(): string {
        // OK: Can call protected method
        return super.protectedMethod();
    }

    public function callPublic(): string {
        // OK: Can call public method
        return super.publicMethod();
    }
}

function main(): void {
    Child child = new Child();
    print(child.tryCallPrivate());
}

main();
