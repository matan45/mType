// Test arithmetic type promotion: int + float = float

function main(): void {
    int intValue = 5;
    float floatValue = 3.14;

    // Arithmetic promotion: int + float should result in float
    float result1 = intValue + floatValue;
    print("Int + Float: " + result1);

    // Test with subtraction
    float result2 = floatValue - intValue;
    print("Float - Int: " + result2);

    // Test with multiplication
    float result3 = intValue * floatValue;
    print("Int * Float: " + result3);

    // Test with division
    float result4 = floatValue / intValue;
    print("Float / Int: " + result4);

    // Complex expression with promotion
    float result5 = (intValue + floatValue) * intValue;
    print("Complex: " + result5);

    // Mixed arithmetic
    int a = 10;
    float b = 2.5;
    float c = a + b - 1.5;
    print("Mixed: " + c);
}

main();
