// Test: Casting large integers to float handles overflow gracefully
// Expected: Values should be converted, potentially losing precision

function testLargeIntToFloat(): void {
    // Test maximum int value (32-bit signed int max)
    int maxInt = 2147483647; // Max 32-bit int
    float maxAsFloat = (float)maxInt;
    print("Max int to float: " + maxAsFloat);

    // Test minimum int value (32-bit signed int min: -2147483648)
    // Note: Can't use -2147483648 directly due to literal parsing, so use -2147483647 - 1
    int minInt = -2147483647 - 1; // Min 32-bit int
    float minAsFloat = (float)minInt;
    print("Min int to float: " + minAsFloat);

    // Test negative large value
    int largeNegative = -999999999;
    float negativeFloat = (float)largeNegative;
    print("Large negative to float: " + negativeFloat);

    // Test positive large value
    int largePositive = 999999999;
    float positiveFloat = (float)largePositive;
    print("Large positive to float: " + positiveFloat);

    // Test that casting works in expressions
    int million = 1000000;
    float computed = (float)(million * million);
    print("Computed large value: " + computed);
}

testLargeIntToFloat();
