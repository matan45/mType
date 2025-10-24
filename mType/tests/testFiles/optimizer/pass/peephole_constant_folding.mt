// Unit test for constant folding optimization
// Expected: All constant expressions should be pre-computed

function testIntegerFolding(): int {
    return 10 + 20 + 30;  // Should fold to: PUSH_INT 60
}

function testFloatFolding(): float {
    return 1.5 * 2.0;  // Should fold to: PUSH_FLOAT 3.0
}

function testComparisonFolding(): bool {
    return 5 < 10;  // Should fold to: PUSH_BOOL true
}

function testLogicalFolding(): bool {
    return true && false;  // Should fold to: PUSH_BOOL false
}

function testUnaryFolding(): int {
    return -(10);  // Should fold to: PUSH_INT -10
}

// Main test runner
print(testIntegerFolding());  // Expected: 60
print(testFloatFolding());    // Expected: 3.0
print(testComparisonFolding());  // Expected: true (1)
print(testLogicalFolding());  // Expected: false (0)
print(testUnaryFolding());    // Expected: -10
print("Constant folding tests completed");
