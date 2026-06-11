// Error: runtime integer modulo by zero (non-constant divisor) must raise
// the "Modulo by zero" runtime error.
function remainder(int a, int b): int {
    return a % b;
}
function main(): void {
    print(remainder(10, 0));
}
main();
