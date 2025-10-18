// Test constant folding for arithmetic operations

function testIntegerArithmetic(): void {
    // Basic arithmetic - should fold to constants
    int a = 5 + 3;        // Should fold to 8
    int b = 10 - 4;       // Should fold to 6
    int c = 3 * 4;        // Should fold to 12
    int d = 15 / 3;       // Should fold to 5
    int e = 10 % 3;       // Should fold to 1

    // Verify results
    print("Testing: 5 + 3 = " + a + " (expected 8)");
    print("Testing: 10 - 4 = " + b + " (expected 6)");
    print("Testing: 3 * 4 = " + c + " (expected 12)");
    print("Testing: 15 / 3 = " + d + " (expected 5)");
    print("Testing: 10 % 3 = " + e + " (expected 1)");

    // Unary operations
    int f = -5;           // Already a literal with unary minus
    int g = -(10);        // Should fold to -10
    int h = +(15);        // Should fold to 15

    print("Testing: -5 = " + f + " (expected -5)");
    print("Testing: -(10) = " + g + " (expected -10)");
    print("Testing: +(15) = " + h + " (expected 15)");

    print("Integer arithmetic tests completed\n");
}

function testFloatArithmetic(): void {
    // Float arithmetic - should fold
    float a = 2.5 + 1.5;  // Should fold to 4.0
    float b = 5.0 - 2.0;  // Should fold to 3.0
    float c = 2.0 * 3.0;  // Should fold to 6.0
    float d = 9.0 / 3.0;  // Should fold to 3.0

    print("Testing: 2.5 + 1.5 = " + a + " (expected 4.0)");
    print("Testing: 5.0 - 2.0 = " + b + " (expected 3.0)");
    print("Testing: 2.0 * 3.0 = " + c + " (expected 6.0)");
    print("Testing: 9.0 / 3.0 = " + d + " (expected 3.0)");

    // Unary float operations
    float e = -2.5;       // Should fold to -2.5
    float f = -(3.5);     // Should fold to -3.5

    print("Testing: -2.5 = " + e + " (expected -2.5)");
    print("Testing: -(3.5) = " + f + " (expected -3.5)");

    print("Float arithmetic tests completed\n");
}

function testMixedArithmetic(): void {
    // Mixed int/float - should promote to float
    float a = 5 + 2.5;    // int + float = float (7.5)
    float b = 3.0 * 2;    // float * int = float (6.0)

    print("Testing: 5 + 2.5 = " + a + " (expected 7.5)");
    print("Testing: 3.0 * 2 = " + b + " (expected 6.0)");

    print("Mixed arithmetic tests completed\n");
}

function testNestedArithmetic(): void {
    // Nested expressions - should fold completely
    int a = (5 + 3) * 2;         // Should fold to 16
    int b = 10 - (3 + 2);        // Should fold to 5
    int c = (20 / 4) + (3 * 2);  // Should fold to 11

    print("Testing: (5 + 3) * 2 = " + a + " (expected 16)");
    print("Testing: 10 - (3 + 2) = " + b + " (expected 5)");
    print("Testing: (20 / 4) + (3 * 2) = " + c + " (expected 11)");

    print("Nested arithmetic tests completed\n");
}

function main(): void {
    print("=== Arithmetic Constant Folding Tests ===\n");

    testIntegerArithmetic();
    testFloatArithmetic();
    testMixedArithmetic();
    testNestedArithmetic();

    print("=== All arithmetic folding tests completed! ===");
}

main();
