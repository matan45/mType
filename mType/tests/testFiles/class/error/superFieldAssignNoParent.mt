// Error: Trying to assign to super.field without a parent class

class Standalone {
    protected int value;

    constructor() {
        value = 42;
    }

    public function tryModifySuper(): void {
        // ERROR: Standalone has no parent class
        super.value = 100;
    }
}

function main(): void {
    Standalone obj = new Standalone();
    obj.tryModifySuper();
    print("Should not reach here");
}

main();
