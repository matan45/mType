// Test: Dead code elimination after throw statement
// Expected output: Before throw
import * from "../../lib/exceptions/Exception.mt";
function test(): void {
    print("Before throw");
    throw new Exception("Error");
    // Everything below should be eliminated
    print("This is unreachable");
    int unused = 100;
}

try {
    test();
} catch (Exception e) {
    print("Caught exception");
}
