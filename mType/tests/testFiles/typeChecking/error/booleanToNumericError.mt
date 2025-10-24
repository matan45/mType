// ERROR: Boolean to numeric conversion

function requiresInt(int value): string {
    return "Value: " + value;
}

function main(): void {
    bool flag = true;

    // ERROR: Cannot convert bool to int
    int result = flag;

    print("This should not execute");
}

main();
