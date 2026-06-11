// Test: add/sub/mul wrap at the int64 boundary (Java-style wrapping semantics).
// INT64_MIN is constructed via 1 << 63 because large literals hit MYT-385.
function main(): void {
    int min = 1 << 63;
    print(min);
    int max = 0 - min - 1;
    print(max);
    print(max + 1);
    print(min - 1);
    print(max * 2);
    print(min * -1);
    print(-min);
}
main();

// Expected output:
// -9223372036854775808
// 9223372036854775807
// -9223372036854775808
// 9223372036854775807
// -2
// -9223372036854775808
// -9223372036854775808
