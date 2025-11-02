// Test: Casting very small floats and underflow scenarios
// Expected: Small values should be handled correctly, rounding to zero if needed

@Script
def testSmallFloatCasting() {
    // Test very small positive float
    var tinyFloat: Float = 0.00000001;
    var asInt: Int = tinyFloat as Int;
    print("Tiny float to int: " + asInt.toString());

    // Test small negative float
    var tinyNegative: Float = -0.00000001;
    var negativeInt: Int = tinyNegative as Int;
    print("Tiny negative to int: " + negativeInt.toString());

    // Test fractional values less than 1
    var fraction: Float = 0.9999;
    var truncated: Int = fraction as Int;
    print("Fraction 0.9999 to int: " + truncated.toString());

    // Test small float to string and back
    var small: Float = 0.0001;
    print("Small float: " + small.toString());

    // Test underflow to zero
    var almostZero: Float = 0.0000000000001;
    var zeroInt: Int = almostZero as Int;
    print("Almost zero to int: " + zeroInt.toString());
}
