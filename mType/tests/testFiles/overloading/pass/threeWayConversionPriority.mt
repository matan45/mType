// Test: Full priority chain EXACT > WIDENING > AUTO_BOXING
import * from "../../lib/primitives/Int.mt";

// Test 1: EXACT vs WIDENING
function classify(int x): string {
    return "exact int";
}

function classify(float x): string {
    return "widened float";
}

// Test 2: WIDENING vs AUTO_BOXING (no exact match available)
function rank(float x): string {
    return "widening";
}

function rank(Int x): string {
    return "auto-boxing";
}

function main(): void {
    print("=== Three Way Conversion Priority ===");

    // EXACT(0) should beat WIDENING(1)
    print(classify(42));

    // Exact float match
    print(classify(3.14));

    // WIDENING(1) should beat AUTO_BOXING(2)
    print(rank(42));
}
main();
