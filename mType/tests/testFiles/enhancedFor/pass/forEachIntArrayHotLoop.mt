// JIT variant: drive the for-each over int[] through a hot function so the
// second CI pass (JIT enabled) actually compiles and OSRs through the loop.
// Pairs with forEachIntArray.mt for the interpreter shape.

function sumArray(int[] arr): int {
    int s = 0;
    for (int x : arr) {
        s = s + x;
    }
    return s;
}

function main(): void {
    int[] arr = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10];
    int N = 50000;
    int total = 0;
    for (int i = 0; i < N; i = i + 1) {
        total = total + sumArray(arr);
    }
    print("total=" + total);
}

main();

// Expected output:
// total=2750000
