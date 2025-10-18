// Test constant folding integration with dead code elimination
// This demonstrates the synergy between constant folding and DCE

function testDeadCodeAfterFolding(): void {
    // Constant folding should convert "if (false)" to dead code that DCE removes
    if (false) {
        print("This should be removed by DCE after constant folding");
        int x = 100;
    }

    // Constant folding should keep only true branch
    if (true) {
        print("This should remain");
    } else {
        print("This should be removed by DCE");
    }

    print("Dead code after folding test completed\n");
}

function testUnreachableAfterTernary(): void {
    // Ternary folding creates opportunities for DCE
    int value = false ? 1 : 2;  // Folds to 2

    print("Testing: false ? 1 : 2 = " + value + " (expected 2)");

    // This if statement can potentially be optimized if value is tracked
    if (value == 1) {
        print("Never executed");
    }

    print("Unreachable after ternary test completed\n");
}

function testComplexOptimizationChain(): void {
    // Multiple passes of CF -> DCE should fully optimize this
    bool condition1 = true && false;   // Folds to false
    bool condition2 = false || true;   // Folds to true

    print("Testing: true && false = " + condition1 + " (expected false)");
    print("Testing: false || true = " + condition2 + " (expected true)");

    if (condition1) {
        // This entire block should be removed
        int a = 100;
        print("Dead code 1");
    }

    if (condition2) {
        // This should remain
        print("Live code");
    } else {
        // This should be removed
        print("Dead code 2");
    }

    print("Complex optimization chain test completed\n");
}

function testIterativeOptimization(): int {
    // This requires fixed-point iteration to fully optimize
    // Pass 1: CF folds (5 > 3) to true
    // Pass 1: DCE removes else branch
    // Pass 2: No changes (fixed point reached)

    if ((5 > 3) && true) {
        print("Optimized through iteration");
        return 1;
    } else {
        print("Should be eliminated");
        return 0;
    }
}

function main(): void {
    print("=== Constant Folding + DCE Integration Tests ===\n");

    testDeadCodeAfterFolding();
    testUnreachableAfterTernary();
    testComplexOptimizationChain();
    int result = testIterativeOptimization();

    print("\nTesting: testIterativeOptimization() returned " + result + " (expected 1)");

    print("\n=== All CF + DCE integration tests completed! ===");
}

main();
