// Test: Casting negative zero (-0.0) vs positive zero (0.0)
// Expected: Both should behave identically in most cases

@Script
def testNegativeZero() {
    // Create negative zero
    var negZero: Float = -0.0;
    var posZero: Float = 0.0;

    print("Negative zero: " + negZero.toString());
    print("Positive zero: " + posZero.toString());

    // Cast both to int
    var negZeroInt: Int = negZero as Int;
    var posZeroInt: Int = posZero as Int;

    print("Negative zero as int: " + negZeroInt.toString());
    print("Positive zero as int: " + posZeroInt.toString());

    // Test equality
    var areEqual: Bool = negZero == posZero;
    print("NegZero == PosZero: " + areEqual.toString());

    // Test in arithmetic
    var negZeroPlus5: Float = negZero + 5.0;
    var posZeroPlus5: Float = posZero + 5.0;

    print("NegZero + 5: " + negZeroPlus5.toString());
    print("PosZero + 5: " + posZeroPlus5.toString());

    // Create negative zero through computation
    var computedNegZero: Float = -1.0 * 0.0;
    print("Computed negative zero: " + computedNegZero.toString());
    var computedNegZeroInt: Int = computedNegZero as Int;
    print("Computed negative zero as int: " + computedNegZeroInt.toString());
}
