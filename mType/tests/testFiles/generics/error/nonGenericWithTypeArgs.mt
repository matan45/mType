// Error: Using type arguments with non-generic class
class SimpleClass {
    int value;
}

function main(): void {
    // Should fail: SimpleClass is not generic but used with type arguments
    SimpleClass<int> obj = new SimpleClass<int>();
}

main();