// Unit test for algebraic simplification
// Expected: Algebraic identities should be applied

function testAddZero(int x): int {
    return x + 0;  // Should become: just x
}

function testMulOne(int x): int {
    return x * 1;  // Should become: just x
}

function testMulZero(int x): int {
    return x * 0;  // Should become: PUSH_INT 0
}

function testSubZero(int x): int {
    return x - 0;  // Should become: just x
}

// Test with actual values
int value = 42;
print(testAddZero(value));   // Expected: 42
print(testMulOne(value));    // Expected: 42
print(testMulZero(value));   // Expected: 0
print(testSubZero(value));   // Expected: 42
print("Algebraic tests completed");
