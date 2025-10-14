// Test: Dead code elimination in nested blocks
// Expected output: 10 20

function outer(): int {
    int x = 10;
    print(x);

    function inner(): int {
        int y = 20;
        print(y);
        return y;
        // Dead code in nested function
        print("Unreachable in inner");
        int z = 30;
    }

    int result = inner();
    return result;
    // Dead code in outer function
    print("Unreachable in outer");
    int w = 40;
}

outer();
