// CANARY (MYT-386, second surface): an assignment inside a constant-false
// branch leaks into execution — `zero` must stay 0 but currently becomes 1.
// Stays failing until MYT-386 lands.
function main(): void {
    int zero = 0;
    if (1 == 2) {
        zero = 1;
    }
    print(zero);
}
main();

// Expected output:
// 0
