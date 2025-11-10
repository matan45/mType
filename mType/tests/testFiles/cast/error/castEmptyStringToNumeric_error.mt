// Test: Attempting to cast empty string to numeric types should fail
// Expected: Runtime error when trying to parse empty string

@Script
def testEmptyStringToNumeric() {
    print("Testing empty string to numeric conversion");

    // Create empty string
    var emptyStr: String = "";
    print("String length: " + emptyStr.length().toString());

    // Attempt to cast empty string to int - should throw error
    print("Attempting to parse empty string to int");
    var resultInt: Int = emptyStr as Int;

    // This should not be reached
    print("Result int: " + resultInt.toString());

    // Also test float conversion (won't be reached due to previous error)
    var resultFloat: Float = emptyStr as Float;
    print("Result float: " + resultFloat.toString());
}
