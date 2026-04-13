// Test: Self-referential object
// Purpose: Verify that GC can handle objects that reference themselves

class SelfRef {
    private SelfRef? self;
    private int value;

    constructor(int v) {
        this.value = v;
        this.self = null;
    }

    public function setSelfReference(): void {
        this.self = this;  // Self-reference
    }

    public function getValue(): int {
        return this.value;
    }
}


function main(): void {
    SelfRef obj = new SelfRef(42);
    obj.setSelfReference();

    print(obj.getValue());
    print("Self-reference test passed");
}
main();