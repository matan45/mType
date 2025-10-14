// Test: Dead code elimination after return statement
// Expected output: 5

function test(): int {
    int x = 5;
    return x;
    // Everything below should be eliminated
    print("This is unreachable");
    int y = 10;
    return y;
}

int result = test();
print(result);
