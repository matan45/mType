// Test: continue inside a try block still runs the finally on every
// iteration. (Break-in-try and continue-in-catch are covered elsewhere;
// continue-in-try was the missing combination.)
function main(): void {
    for (int i = 0; i < 4; i++) {
        try {
            if (i % 2 == 0) {
                continue;
            }
            print("body " + i);
        } finally {
            print("finally " + i);
        }
    }
    print("done");
}
main();

// Expected output:
// finally 0
// body 1
// finally 1
// finally 2
// body 3
// finally 3
// done
