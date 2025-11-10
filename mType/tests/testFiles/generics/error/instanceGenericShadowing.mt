// Test: Error - Method generic type parameter shadows class-level parameter
// Expected error: Method generic type parameter 'T' shadows class-level type parameter

class Box<T> {
    T data;

    public constructor(T value) {
        this.data = value;
    }

    // ERROR: Method-level T shadows class-level T
    public function <T> bad(T value): T {
        return value;
    }
}

function main(): void {
    // This should fail during compilation
    Box<int> box = new Box<int>(42);
}

main();
