// Test that void primitive type is rejected in generics
// This should fail with "Invalid type arguments" error

class VoidContainer<T> {
    T content;

    function setContent(T val): void {
        content = val;
    }

    function getContent(): T {
        return content;
    }
}

function main(): void {
    // This should fail - void is a primitive type
    VoidContainer<void> voidContainer = new VoidContainer<void>();
    print("This should not be reached");
}

main();