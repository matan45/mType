// Error: Trying to assign to super.field when parent (Object) has no such field

class Standalone {
    protected int value;

    constructor() {
        value = 42;
    }

    public function tryModifySuper(): void {
        // ERROR: Object has no 'value' field
        super.value = 100;
    }
}

function main(): void {
    Standalone obj = new Standalone();
    obj.tryModifySuper();
    print("Should not reach here");
}

main();
