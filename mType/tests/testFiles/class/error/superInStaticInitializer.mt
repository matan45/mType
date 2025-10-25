// Test: Super access in static field initializer
// Expected: Should fail - cannot use super in static initializer

class Parent {
    protected int value = 10;
    protected string message = "parent";

    public function getValue(): int {
        return this.value;
    }
}

class Child extends Parent {
    constructor(): super() {
    }

    // ERROR: Cannot use super in static field initializer
    public static int staticField = super.value;

    // ERROR: Cannot call super methods in static initializer
    public static string staticMessage = super.message;
}

function main(): void {
    print("Static field: " + Child.staticField);
}

main();
