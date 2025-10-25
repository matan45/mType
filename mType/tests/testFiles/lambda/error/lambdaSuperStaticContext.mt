// Test: Lambda with super in static context
// Expected: Should fail - cannot use super in static context

interface Function {
    function apply(): string;
}

class Parent {
    public function parentMethod(): string {
        return "parent instance method";
    }

    protected int value = 10;
}

class Child extends Parent {
    constructor(): super() {
    }

    // ERROR: Static method trying to create lambda with super access
    public static function createLambdaWithSuper(): Function {
        return () -> super.parentMethod();
    }

    // ERROR: Static method trying to create lambda accessing super field
    public static function createLambdaWithSuperField(): Function {
        return () -> "Value: " + super.value;
    }
}

function main(): void {
    Function lambda = Child.createLambdaWithSuper();
    print(lambda.apply());
}

main();
