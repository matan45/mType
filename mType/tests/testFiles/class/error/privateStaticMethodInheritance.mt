// Test: Private static method access in child class
// Expected: Should fail - cannot access parent's private static method

class Parent {
    private static function secretStatic(): string {
        return "Secret static from parent";
    }

    protected static function protectedStatic(): string {
        return "Protected static from parent";
    }

    public static function callSecret(): string {
        // OK: Parent can call its own private static method
        return secretStatic();
    }
}

class Child extends Parent {
    constructor(): super() {
    }

    public static function tryCallSecretStatic(): string {
        // ERROR: Cannot access parent's private static method
        return secretStatic();
    }

    public static function canCallProtectedStatic(): string {
        // OK: Can call protected static method
        return protectedStatic();
    }

    public function tryCallSecretFromInstance(): string {
        // ERROR: Cannot access private static method from parent
        return secretStatic();
    }
}

function main(): void {
    print(Child.tryCallSecretStatic());
}

main();
