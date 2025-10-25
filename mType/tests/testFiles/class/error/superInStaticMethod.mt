// Test: Super access in static method
// Expected: Should fail - cannot use super in static context

class Parent {
    public function instanceMethod(): string {
        return "parent instance method";
    }

    public static function staticMethod(): string {
        return "parent static method";
    }

    protected int value = 10;
}

class Child extends Parent {
    constructor(): super() {
    }

    // ERROR: Cannot use super in static method to access instance members
    public static function testSuperInStatic(): string {
        return super.instanceMethod();
    }

    // ERROR: Cannot access super fields in static method
    public static function accessSuperField(): int {
        return super.value;
    }
}

function main(): void {
    print(Child.testSuperInStatic());
}

main();
