// Test: Casting positive and negative infinity to integer
// Expected: Infinity values should be handled gracefully

@Script
def testInfinityCasting() {
    // Create positive infinity
    var one: Float = 1.0;
    var zero: Float = 0.0;
    var posInf: Float = one / zero;

    print("Positive infinity: " + posInf.toString());

    // Cast positive infinity to int
    var posInfInt: Int = posInf as Int;
    print("Positive infinity as int: " + posInfInt.toString());

    // Create negative infinity
    var negOne: Float = -1.0;
    var negInf: Float = negOne / zero;

    print("Negative infinity: " + negInf.toString());

    // Cast negative infinity to int
    var negInfInt: Int = negInf as Int;
    print("Negative infinity as int: " + negInfInt.toString());

    // Test infinity in comparisons
    var isPosInfinity: Bool = posInf > 1000000.0;
    print("PosInf > 1000000: " + isPosInfinity.toString());

    var isNegInfinity: Bool = negInf < -1000000.0;
    print("NegInf < -1000000: " + isNegInfinity.toString());
}
