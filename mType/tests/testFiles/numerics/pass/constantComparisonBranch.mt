// CANARY (MYT-386): if/while conditions comparing two constants always take
// the then-branch because ConstantFoldingPattern::applyComparison emits
// PUSH_BOOL with a constant-pool index while handlePushBool reads the operand
// as the inline boolean. Pins the CORRECT branch selection; stays failing
// until MYT-386 lands.
function main(): void {
    if (1 == 2) {
        print("then");
    } else {
        print("else");
    }
    if (3 < 2) {
        print("then");
    } else {
        print("else");
    }
    int n = 0;
    while (1 == 2) {
        n = n + 1;
        if (n > 3) {
            break;
        }
    }
    print(n);
}
main();

// Expected output:
// else
// else
// 0
