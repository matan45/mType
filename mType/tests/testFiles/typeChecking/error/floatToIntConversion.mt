// ERROR: Float to int conversion without explicit cast (truncation)

function requiresInt(int value): string {
    return "Received int: " + value;
}

function main(): void {
    float floatValue = 3.14;

    // ERROR: Cannot implicitly convert float to int (truncation risk)
    int intValue = floatValue;

    print("This should not execute");
}

main();
