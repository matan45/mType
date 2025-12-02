// Test: Casting large integers to float handles precision correctly
// Expected: Values should be converted, potentially losing precision for very large values

function testLargeIntToFloat(): void {
    // Test maximum 32-bit int value
    int maxInt = 2147483647; // Max 32-bit int
    float maxAsFloat = (float)maxInt;
    print("Max int to float: " + maxAsFloat);

    // Test minimum 32-bit int value
    int minInt = -2147483648; // Min 32-bit int
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

    // Test that casting works in expressions (64-bit: 1 trillion, no overflow)
    int million = 1000000;
    float computed = (float)(million * million);
    print("Computed large value: " + computed);
}

testLargeIntToFloat();
