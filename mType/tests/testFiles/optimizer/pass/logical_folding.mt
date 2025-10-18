// Test constant folding for logical operations

function testLogicalOperations(): void {
    // AND operations
    bool a = true && true;      // Should fold to true
    bool b = true && false;     // Should fold to false
    bool c = false && true;     // Should fold to false
    bool d = false && false;    // Should fold to false

    print("Testing: true && true = " + a + " (expected true)");
    print("Testing: true && false = " + b + " (expected false)");
    print("Testing: false && true = " + c + " (expected false)");
    print("Testing: false && false = " + d + " (expected false)");

    // OR operations
    bool e = true || true;      // Should fold to true
    bool f = true || false;     // Should fold to true
    bool g = false || true;     // Should fold to true
    bool h = false || false;    // Should fold to false

    print("Testing: true || true = " + e + " (expected true)");
    print("Testing: true || false = " + f + " (expected true)");
    print("Testing: false || true = " + g + " (expected true)");
    print("Testing: false || false = " + h + " (expected false)");

    // NOT operations
    bool i = !true;             // Should fold to false
    bool j = !false;            // Should fold to true

    print("Testing: !true = " + i + " (expected false)");
    print("Testing: !false = " + j + " (expected true)");

    print("Logical operations tests completed\n");
}

function testComplexLogicalExpressions(): void {
    // Nested logical expressions
    bool a = (true && true) || false;       // Should fold to true
    bool b = (false || false) && true;      // Should fold to false
    bool c = !(true && false);              // Should fold to true
    bool d = !(!true);                      // Should fold to true

    print("Testing: (true && true) || false = " + a + " (expected true)");
    print("Testing: (false || false) && true = " + b + " (expected false)");
    print("Testing: !(true && false) = " + c + " (expected true)");
    print("Testing: !(!true) = " + d + " (expected true)");

    print("Complex logical expressions tests completed\n");
}

function testLogicalWithComparisons(): void {
    // Logical operations combined with comparisons
    bool a = (5 > 3) && (10 < 20);         // Should fold to true
    bool b = (5 == 5) || (3 > 10);         // Should fold to true
    bool c = !(5 < 3);                     // Should fold to true

    print("Testing: (5 > 3) && (10 < 20) = " + a + " (expected true)");
    print("Testing: (5 == 5) || (3 > 10) = " + b + " (expected true)");
    print("Testing: !(5 < 3) = " + c + " (expected true)");

    print("Logical with comparisons tests completed\n");
}

function main(): void {
    print("=== Logical Constant Folding Tests ===\n");

    testLogicalOperations();
    testComplexLogicalExpressions();
    testLogicalWithComparisons();

    print("=== All logical folding tests completed! ===");
}

main();
