// Test enhanced for-loop over primitive bool[] array
function main(): void {
    print("Testing enhanced for-loop over bool[]:");

    bool[] flags = [true, false, true, false, true];
    int trueCount = 0;
    for (bool b : flags) {
        print(b);
        if (b) {
            trueCount = trueCount + 1;
        }
    }
    print("True count: " + trueCount);

    print("for-each over bool[] test passed!");
}

main();
