// Test enhanced for-loop over primitive int[] array literal
function main(): void {
    print("Testing enhanced for-loop over int[]:");

    int[] arr = [10, 20, 30, 40, 50];
    int sum = 0;
    for (int x : arr) {
        print(x);
        sum = sum + x;
    }
    print("Sum: " + sum);

    // Empty primitive int[] should not enter the body
    int[] empty = new int[0];
    int emptyCount = 0;
    for (int y : empty) {
        emptyCount = emptyCount + 1;
    }
    print("Empty count: " + emptyCount);

    print("for-each over int[] test passed!");
}

main();
