// No matching overload - should fail at compile time
function process(int x) {
    println("int: " + x);
}

function process(String s) {
    println("String: " + s);
}

@Script
function main() {
    process(42);         // OK - matches process(int)
    process("Hello");    // OK - matches process(String)

    // This should fail - no overload accepts (int, int)
    process(10, 20);     // ERROR: No matching overload for process(int, int)
}
