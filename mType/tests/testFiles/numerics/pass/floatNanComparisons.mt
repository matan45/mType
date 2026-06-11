// Test: IEEE NaN comparison semantics. NaN is produced via inf - inf and
// inf / inf (exponent literals are unsupported, so inf comes from repeated
// squaring). Discriminating booleans are printed, never raw NaN/inf text.
function main(): void {
    float big = 1000000.0;
    for (int i = 0; i < 10; i++) {
        big = big * big;
    }
    float nan = big - big;
    print(nan == nan);
    print(nan != nan);
    float nan2 = big / big;
    print(nan2 != nan2);
    print(nan < 0.0);
    print(nan > 0.0);
}
main();

// Expected output:
// false
// true
// true
// false
// false
