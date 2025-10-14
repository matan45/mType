// Test: Dead code elimination in if/else branches
// Expected output: true branch

bool condition = true;

if (condition) {
    print("true branch");
    return;
    // Dead code in then branch
    print("Unreachable in then");
} else {
    print("false branch");
    return;
    // Dead code in else branch
    print("Unreachable in else");
}

// This is also unreachable
print("After if-else");
