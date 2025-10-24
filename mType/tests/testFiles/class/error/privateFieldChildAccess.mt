// ERROR: Child accessing parent's private field

class Parent {
    private int secretValue;

    public function Parent() {
        secretValue = 99;
    }

    public function getSecret(): int {
        return secretValue;
    }
}

class Child extends Parent {
    public function Child() {
        super();
    }

    public function accessSecret(): int {
        // ERROR: Cannot access private field from parent
        return secretValue;
    }
}

function main(): void {
    Child child = new Child();
    int value = child.accessSecret();
    print("This should not execute");
}

main();
