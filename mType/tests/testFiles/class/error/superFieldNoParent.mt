// Error: Trying to access super.field when parent (Object) has no such field

class Standalone {
    protected int value;

    constructor() {
        value = 42;
    }

    public function tryAccessSuper(): int {
        // ERROR: Object has no 'value' field
        return super.value;
    }
}

function main(): void {
    Standalone obj = new Standalone();
    print(obj.tryAccessSuper());
}

main();
