// Test: Single candidate with widening conversion should still be viable
function onlyFloat(float x): string {
    return "float: " + x;
}

function main(): void {
    print("=== Single Widening Fallback ===");

    // Only float overload exists, int should widen to float
    print(onlyFloat(42));
    print(onlyFloat(3.14));
}
main();
