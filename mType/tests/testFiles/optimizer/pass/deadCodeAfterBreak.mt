// Test: Dead code elimination after break statement
// Expected output: 0 1 2

for (int i = 0; i < 10; i++) {
    print(i);
    if (i == 2) {
        break;
        // Everything below should be eliminated
        print("This is unreachable");
        int unused = 100;
    }
}
