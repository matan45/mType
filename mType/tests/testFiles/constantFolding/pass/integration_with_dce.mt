// Test constant folding integration with dead code elimination
// This demonstrates the synergy between constant folding and DCE

function testDeadCodeAfterFolding() {
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

    return 0;
}

function testUnreachableAfterTernary() {
    // Ternary folding creates opportunities for DCE
    int value = false ? 1 : 2;  // Folds to 2

    // This if statement can potentially be optimized if value is tracked
    if (value == 1) {
        print("Never executed");
    }

    return 0;
}

function testComplexOptimizationChain() {
    // Multiple passes of CF -> DCE should fully optimize this
    bool condition1 = true && false;   // Folds to false
    bool condition2 = false || true;   // Folds to true

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

    return 0;
}

function testIterativeOptimization() {
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

function main() {
    testDeadCodeAfterFolding();
    testUnreachableAfterTernary();
    testComplexOptimizationChain();
    int result = testIterativeOptimization();

    assert(result == 1);

    print("All CF + DCE integration tests passed!");
    return 0;
}
