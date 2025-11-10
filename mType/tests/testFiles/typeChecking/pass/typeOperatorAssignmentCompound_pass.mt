// Test compound assignment operators with type checking

@Script
function main(): void {
    // Test += operator
    int intValue = 10;
    intValue += 5;
    print("intValue after +=: " + intValue);

    float floatValue = 3.14;
    floatValue += 2.86;
    print("floatValue after +=: " + floatValue);

    // Test with type promotion (int += float results in float-like behavior)
    int mixedValue = 10;
    mixedValue += 5;
    print("mixedValue after += int: " + mixedValue);

    // Test -= operator
    int subValue = 20;
    subValue -= 7;
    print("subValue after -=: " + subValue);

    float floatSub = 10.5;
    floatSub -= 3.5;
    print("floatSub after -=: " + floatSub);

    // Test *= operator
    int mulValue = 5;
    mulValue *= 3;
    print("mulValue after *=: " + mulValue);

    float floatMul = 2.5;
    floatMul *= 4.0;
    print("floatMul after *=: " + floatMul);

    // Test /= operator
    int divValue = 20;
    divValue /= 4;
    print("divValue after /=: " + divValue);

    float floatDiv = 10.0;
    floatDiv /= 2.5;
    print("floatDiv after /=: " + floatDiv);

    // String concatenation with +=
    string text = "Hello";
    text += " World";
    print("text after +=: " + text);

    // Multiple compound operations
    int counter = 0;
    counter += 10;
    counter -= 3;
    counter *= 2;
    counter /= 7;
    print("counter after multiple ops: " + counter);
}

main();
