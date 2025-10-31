// ERROR: Override protected final method

class Parent {
    protected final function process(): string {
        return "Parent protected final method";
    }
}

class Child extends Parent {
    // ERROR: Cannot override final method even if it's protected
    protected function process(): string {
        return "Child override attempt";
    }
}

function main(): void {
    Child child = new Child();
    print("This should not execute");
}

main();
