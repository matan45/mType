// Test: Calling static method via super
// Expected: Should fail - cannot call static methods via super keyword

class Parent {
    public static function staticMethod(): string {
        return "parent static method";
    }

    public static int staticValue = 100;

    public function instanceMethod(): string {
        return "instance";
    }
}

class Child extends Parent {
    constructor(): super() {
    }

    public static function overriddenStatic(): string {
        return "child static";
    }

    // ERROR: Cannot call static method via super - should use Parent.staticMethod()
    public static function testSuperStatic(): string {
        return super.staticMethod();
    }

    // ERROR: Cannot access static field via super
    public function accessStaticViaSuper(): int {
        return super.staticValue;
    }

    // This should also fail even in instance context
    public function callStaticViaSuper(): string {
        return super.staticMethod();
    }
}

function main(): void {
    print(Child.testSuperStatic());
}

main();
