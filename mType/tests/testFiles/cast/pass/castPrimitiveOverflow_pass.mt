// Test: Casting large integers to float handles overflow gracefully
// Expected: Values should be converted, potentially losing precision

@Script
def testLargeIntToFloat() {
    // Test very large integer to float conversion
    var largeInt: Int = 9007199254740992; // 2^53, beyond float precise range
    var asFloat: Float = largeInt as Float;
    print("Large int to float: " + asFloat.toString());

    // Test maximum int value
    var maxInt: Int = 2147483647; // Max 32-bit int
    var maxAsFloat: Float = maxInt as Float;
    print("Max int to float: " + maxAsFloat.toString());

    // Test negative large value
    var largeNegative: Int = -999999999;
    var negativeFloat: Float = largeNegative as Float;
    print("Large negative to float: " + negativeFloat.toString());

    // Test that casting works in expressions
    var computed: Float = (1000000 * 1000000) as Float;
    print("Computed large value: " + computed.toString());
}
