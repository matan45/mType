// Test type checking in logical operators with short-circuit evaluation

@Script
function main(): void {
    // Test && operator - should short-circuit
    bool result1 = false && true;
    print("false && true: " + result1);

    bool result2 = true && true;
    print("true && true: " + result2);

    bool result3 = true && false;
    print("true && false: " + result3);

    // Test || operator - should short-circuit
    bool result4 = true || false;
    print("true || false: " + result4);

    bool result5 = false || true;
    print("false || true: " + result5);

    bool result6 = false || false;
    print("false || false: " + result6);

    // Complex logical expressions
    int a = 5;
    int b = 10;
    bool result7 = (a < b) && (b > 0);
    print("(a < b) && (b > 0): " + result7);

    bool result8 = (a > b) || (b > 0);
    print("(a > b) || (b > 0): " + result8);

    // Nested logical operations
    bool result9 = (a < b) && ((b > 0) || (a == 0));
    print("Nested: " + result9);

    // Comparison with logical operators
    bool result10 = (a + b > 10) && (a * b < 100);
    print("Complex: " + result10);
}

main();
