// Test: Dead code elimination after throw statement
// Expected output: Before throw

function test(): void {
    print("Before throw");
    throw "Error";
    // Everything below should be eliminated
    print("This is unreachable");
    int unused = 100;
}

try {
    test();
} catch (Exception e) {
    print("Caught exception");
}
