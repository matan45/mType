// Test: Dead code elimination in if/else branches
// Expected output: true branch

bool condition = true;
function test():void{

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

}

test();
// This is also unreachable
print("After if-else");
