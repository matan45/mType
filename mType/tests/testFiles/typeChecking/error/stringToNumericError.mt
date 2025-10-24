// ERROR: String to numeric conversion without explicit parse

function requiresInt(int value): string {
    return "Value: " + value;
}

function main(): void {
    string numStr = "42";

    // ERROR: Cannot implicitly convert string to int
    int result = numStr;

    print("This should not execute");
}

main();
