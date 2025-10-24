// Error: Trying to access super.field without a parent class

class Standalone {
    protected int value;

    constructor() {
        value = 42;
    }

    public function tryAccessSuper(): int {
        // ERROR: Standalone has no parent class
        return super.value;
    }
}

function main(): void {
    Standalone obj = new Standalone();
    print(obj.tryAccessSuper());
}

main();
