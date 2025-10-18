// Test constant folding for ternary operators and string concatenation

function testTernaryFolding(): void {
    // Ternary with constant condition - should fold to one branch
    int a = true ? 10 : 20;     // Should fold to 10
    int b = false ? 10 : 20;    // Should fold to 20

    print("Testing: true ? 10 : 20 = " + a + " (expected 10)");
    print("Testing: false ? 10 : 20 = " + b + " (expected 20)");

    // Nested ternary
    int c = true ? (false ? 1 : 2) : 3;   // Should fold to 2

    print("Testing: true ? (false ? 1 : 2) : 3 = " + c + " (expected 2)");

    // Ternary with expressions in branches
    int d = true ? (5 + 3) : (10 - 2);    // Should fold to 8

    print("Testing: true ? (5 + 3) : (10 - 2) = " + d + " (expected 8)");

    print("Ternary folding tests completed\n");
}

function teststringConcatenation(): void {
    // string concatenation - should fold
    string a = "hello" + " world";          // Should fold to "hello world"
    string b = "foo" + "bar";               // Should fold to "foobar"

    print("Testing: \"hello\" + \" world\" = " + a + " (expected \"hello world\")");
    print("Testing: \"foo\" + \"bar\" = " + b + " (expected \"foobar\")");

    // string concatenation with numbers
    string c = "value: " + 42;              // Should fold to "value: 42" (if supported)
    string d = "result: " + true;           // Should fold to "result: true" (if supported)

    print("Testing: \"value: \" + 42 = " + c);
    print("Testing: \"result: \" + true = " + d);

    print("string concatenation tests completed\n");
}

function testMixedOperations(): void {
    // Complex expressions combining multiple foldable operations
    int a = (5 + 3) > (2 * 2) ? 100 : 200;    // (8 > 4) ? 100 : 200 -> 100

    print("Testing: (5 + 3) > (2 * 2) ? 100 : 200 = " + a + " (expected 100)");

    // Logical and comparison folding in ternary
    int b = (true && false) ? 1 : 2;          // false ? 1 : 2 -> 2

    print("Testing: (true && false) ? 1 : 2 = " + b + " (expected 2)");

    print("Mixed operations tests completed\n");
}

function main(): void {
    print("=== Ternary and string Constant Folding Tests ===\n");

    testTernaryFolding();
    teststringConcatenation();
    testMixedOperations();

    print("=== All ternary and string folding tests completed! ===");
}

main();
