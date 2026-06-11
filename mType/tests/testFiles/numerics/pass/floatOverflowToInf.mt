// Test: float overflow saturates to infinity (does not throw or wrap).
// Discriminating booleans are printed, never raw inf text.
function main(): void {
    float big = 1000000.0;
    for (int i = 0; i < 10; i++) {
        big = big * big;
    }
    print(big > 1.0);
    float doubled = big * 2.0;
    print(doubled > 1.0);
    print(big == doubled);
    float negInf = 0.0 - big;
    print(negInf < 0.0);
    print(negInf == big);
}
main();

// Expected output:
// true
// true
// true
// true
// false
