// Test: Attempting to cast invalid string to float should fail
// Expected: Runtime error when trying to parse invalid float string

@Script
def testInvalidStringToFloat() {
    print("Testing invalid string to float conversion");

    // Test with alphabetic characters
    var invalidStr1: String = "abc123";
    print("Attempting to parse: " + invalidStr1);

    // This should cause a runtime error
    var result: Float = invalidStr1 as Float;

    // This should not be reached
    print("Result: " + result.toString());
}
