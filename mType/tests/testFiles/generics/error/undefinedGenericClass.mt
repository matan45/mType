// Error: Using undefined generic class
function main(): void {
    // Should fail: UnknownGeneric class does not exist
    UnknownGeneric<int> obj = new UnknownGeneric<int>();
}

main();