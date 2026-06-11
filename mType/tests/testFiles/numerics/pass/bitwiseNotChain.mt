// Test: unary bitwise NOT on signed ints, including chained application.
function main(): void {
    print(~0);
    print(~-1);
    print(~5);
    print(~~5);
    print(~~~7);
}
main();

// Expected output:
// -1
// 0
// -6
// 5
// -8
