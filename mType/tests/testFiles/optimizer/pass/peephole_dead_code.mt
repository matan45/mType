// Unit test for dead code elimination
// Expected: Unused expressions should be removed

function testDeadCode(): int {
    int x = 10;

    // These expressions do nothing - should be eliminated
    20 + 30;
    5 * 10;
    true && false;

    return x;  // Should just return x
}

function testPushPop(): int {
    int x = 5;
    // The following pattern should be eliminated: PUSH, POP
    int y = 10;
    return x;
}

print(testDeadCode());    // Expected: 10
print(testPushPop());      // Expected: 5
print("Dead code tests completed");
