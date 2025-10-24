// ERROR: Access modifier narrowing in override (public → private)

class Parent {
    public function process(): string {
        return "Parent process";
    }
}

class Child extends Parent {
    // ERROR: Cannot narrow public to private
    private function process(): string {
        return "Child process";
    }
}

function main(): void {
    Child child = new Child();
    print("This should not execute");
}

main();
