// No matching overload - should fail at compile time
function process(int x): void {
    print("int: " + x);
}

function process(string s): void {
    print("string: " + s);
}

function main(): void {
    process(42);         // OK - matches process(int)
    process("Hello");    // OK - matches process(string)

    // This should fail - no overload accepts (int, int)
    process(10, 20);     // ERROR: No matching overload for process(int, int)
}
