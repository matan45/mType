// Error: runtime integer division by zero (non-constant divisor) must raise
// the "Division by zero" runtime error.
function divide(int a, int b): int {
    return a / b;
}
function main(): void {
    print(divide(10, 0));
}
main();
