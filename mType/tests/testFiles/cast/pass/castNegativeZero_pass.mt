// Test: Casting negative zero (-0.0) vs positive zero (0.0)
// Expected: Both should behave identically in most cases

function testNegativeZero(): void {
    // Create negative zero
    float negZero = -0.0;
    float posZero = 0.0;

    print("Negative zero: " + negZero);
    print("Positive zero: " + posZero);

    // Cast both to int
    int negZeroInt = (int)negZero;
    int posZeroInt = (int)posZero;

    print("Negative zero as int: " + negZeroInt);
    print("Positive zero as int: " + posZeroInt);

    // Test equality
    bool areEqual = negZero == posZero;
    print("NegZero == PosZero: " + areEqual);

    // Test in arithmetic
    float negZeroPlus5 = negZero + 5.0;
    float posZeroPlus5 = posZero + 5.0;

    print("NegZero + 5: " + negZeroPlus5);
    print("PosZero + 5: " + posZeroPlus5);

    // Create negative zero through computation
    float computedNegZero = -1.0 * 0.0;
    print("Computed negative zero: " + computedNegZero);
    int computedNegZeroInt = (int)computedNegZero;
    print("Computed negative zero as int: " + computedNegZeroInt);
}

testNegativeZero();
