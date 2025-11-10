// Test: Casting very small floats and underflow scenarios
// Expected: Small values should be handled correctly, rounding to zero if needed

function testSmallFloatCasting(): void {
    // Test very small positive float
    float tinyFloat = 0.00000001;
    int asInt = (int)tinyFloat;
    print("Tiny float to int: " + asInt);

    // Test small negative float
    float tinyNegative = -0.00000001;
    int negativeInt = (int)tinyNegative;
    print("Tiny negative to int: " + negativeInt);

    // Test fractional values less than 1
    float fraction = 0.9999;
    int truncated = (int)fraction;
    print("Fraction 0.9999 to int: " + truncated);

    // Test small float to string and back
    float small = 0.0001;
    print("Small float: " + small);

    // Test underflow to zero
    float almostZero = 0.0000000000001;
    int zeroInt = (int)almostZero;
    print("Almost zero to int: " + zeroInt);
}

testSmallFloatCasting();
