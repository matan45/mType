// Test: Generic inheritance with super type mismatch
// Expected: Should fail - type parameter mismatch between child and parent

class Parent<T> {
    public function process(T item): T {
        return item;
    }

    public function getValue(): T {
        return null;
    }
}

class Child<U> extends Parent<string> {
    private U data;

    constructor(U d) {
        this.data = d;
    }

    public function test(): void {
        // ERROR: Type mismatch - trying to assign string (from super.process) to U
        U result = super.process("test");
        print("This should not execute: " + result);
    }

    public function test2(): void {
        // ERROR: Type mismatch - super.getValue returns string, not U
        U value = super.getValue();
        print("This should not execute: " + value);
    }
}

function main(): void {
    Child<int> child = new Child<int>(42);
    child.test();
}

main();
