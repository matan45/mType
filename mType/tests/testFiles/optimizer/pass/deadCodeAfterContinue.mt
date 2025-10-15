// Test: Dead code elimination after continue statement
// Expected output: 0 2 4 6 8

for (int i = 0; i < 10; i++) {
    if (i % 2 == 1) {
        continue;
        // Everything below should be eliminated
        print("This is unreachable");
        int unused = 100;
    }
    print(i);
}
