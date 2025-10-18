// Test constant folding for arithmetic operations

function testIntegerArithmetic() {
    // Basic arithmetic - should fold to constants
    int a = 5 + 3;        // Should fold to 8
    int b = 10 - 4;       // Should fold to 6
    int c = 3 * 4;        // Should fold to 12
    int d = 15 / 3;       // Should fold to 5
    int e = 10 % 3;       // Should fold to 1

    // Verify results
    assert(a == 8);
    assert(b == 6);
    assert(c == 12);
    assert(d == 5);
    assert(e == 1);

    // Unary operations
    int f = -5;           // Already a literal with unary minus
    int g = -(10);        // Should fold to -10
    int h = +(15);        // Should fold to 15

    assert(f == -5);
    assert(g == -10);
    assert(h == 15);

    return 0;
}

function testFloatArithmetic() {
    // Float arithmetic - should fold
    float a = 2.5 + 1.5;  // Should fold to 4.0
    float b = 5.0 - 2.0;  // Should fold to 3.0
    float c = 2.0 * 3.0;  // Should fold to 6.0
    float d = 9.0 / 3.0;  // Should fold to 3.0

    assert(a == 4.0);
    assert(b == 3.0);
    assert(c == 6.0);
    assert(d == 3.0);

    // Unary float operations
    float e = -2.5;       // Should fold to -2.5
    float f = -(3.5);     // Should fold to -3.5

    assert(e == -2.5);
    assert(f == -3.5);

    return 0;
}

function testMixedArithmetic() {
    // Mixed int/float - should promote to float
    float a = 5 + 2.5;    // int + float = float (7.5)
    float b = 3.0 * 2;    // float * int = float (6.0)

    assert(a == 7.5);
    assert(b == 6.0);

    return 0;
}

function testNestedArithmetic() {
    // Nested expressions - should fold completely
    int a = (5 + 3) * 2;         // Should fold to 16
    int b = 10 - (3 + 2);        // Should fold to 5
    int c = (20 / 4) + (3 * 2);  // Should fold to 11

    assert(a == 16);
    assert(b == 5);
    assert(c == 11);

    return 0;
}

function main() {
    testIntegerArithmetic();
    testFloatArithmetic();
    testMixedArithmetic();
    testNestedArithmetic();

    print("All arithmetic folding tests passed!");
    return 0;
}
