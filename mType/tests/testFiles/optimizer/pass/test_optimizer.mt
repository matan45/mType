// Test program for peephole optimizer
// This program contains patterns that should be optimized

function testConstantFolding(): int {
    // Should be folded to: PUSH_INT 30
    int result = 10 + 20;
    return result;
}

function testDeadCode(): int {
    int x = 5;
    10 + 20;  // Result not used - should be eliminated
    return x;
}

function testAlgebraic(): int {
    int x = 100;
    int result = x + 0;  // Should become just: x
    result = result * 1; // Should become just: result
    return result;
}

function testStrengthReduction(): int {
    int x = 50;
    int result = x / 1;  // Should become just: x
    return result;
}

function testComplexExpression(): int {
    // Multiple optimizations possible
    int a = 5 + 3;      // Constant folding: 8
    int b = a * 1;      // Algebraic: just a
    int c = b + 0;      // Algebraic: just b
    int d = c / 1;      // Strength reduction: just c
    return d;
}

function testComparisons(): bool {
    // Should be folded to: PUSH_BOOL true
    bool result = 10 < 20;
    return result;
}

function testLogical(): bool {
    // Should be folded to: PUSH_BOOL false
    bool result = true && false;
    return result;
}

function testNested(): int {
    int x = (10 + 5) * 2;  // Should fold to: 30
    int y = x - 0;         // Should simplify to: x
    return y;
}

// Main execution
print("Testing Peephole Optimizer");
print(testConstantFolding());
print(testDeadCode());
print(testAlgebraic());
print(testStrengthReduction());
print(testComplexExpression());
print(testComparisons());
print(testLogical());
print(testNested());
print("All tests completed");
