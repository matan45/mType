// ERROR: Override parent's final method

class Parent {
    public final function process(): string {
        return "Parent final method";
    }
}

class Child extends Parent {
    // ERROR: Cannot override final method
    public function process(): string {
        return "Child override attempt";
    }
}

function main(): void {
    Child child = new Child();
    print("This should not execute");
}

main();
