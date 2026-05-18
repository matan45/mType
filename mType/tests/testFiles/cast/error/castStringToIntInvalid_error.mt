// Test: Attempting to cast invalid string to integer should fail
// Expected: Runtime error when trying to parse invalid integer string

def testInvalidStringToInt() {
    print("Testing invalid string to int conversion");

    // Attempt to cast invalid string to int - should throw error
    var invalidStr: String = "not_a_number";
    print("Attempting to parse: " + invalidStr);

    // This should cause a runtime error
    var result: Int = invalidStr as Int;

    // This should not be reached
    print("Result: " + result.toString());
}
