// Error: shift amounts above 63 are rejected at runtime
// ("Shift amount must be between 0 and 63").
function shiftBy(int a, int s): int {
    return a << s;
}
function main(): void {
    print(shiftBy(1, 64));
}
main();
