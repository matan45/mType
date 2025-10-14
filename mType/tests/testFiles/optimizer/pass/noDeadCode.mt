// Test: No dead code to eliminate (all code is reachable)
// Expected output: 1 2 3 4 5

function test(): void {
    print(1);
    print(2);
    print(3);
    print(4);
    print(5);
}

test();
