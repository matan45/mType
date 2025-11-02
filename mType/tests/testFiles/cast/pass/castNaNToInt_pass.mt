// Test: Casting NaN (Not a Number) to integer
// Expected: NaN should convert to 0 or be handled gracefully

@Script
def testNaNCasting() {
    // Create NaN by dividing 0.0 by 0.0
    var zero: Float = 0.0;
    var nan: Float = zero / zero;

    print("NaN value: " + nan.toString());

    // Cast NaN to int - should handle gracefully
    var nanAsInt: Int = nan as Int;
    print("NaN as int: " + nanAsInt.toString());

    // Test NaN in comparisons
    var isNaN: Bool = nan != nan; // NaN != NaN is true
    print("NaN != NaN: " + isNaN.toString());

    // Test NaN to string
    print("NaN string representation: " + nan.toString());
}
