// Test nullable primitive boxing error
// Validates that primitives cannot directly hold null without boxing

function processValue(int value): int {
    return value * 2;
}

function main(): void {
    // This should fail: attempting to assign null to primitive int
    int x = null;  // Error: primitives cannot be null

    processValue(x);
}

main();
