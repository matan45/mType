// Test that wildcard import doesn't expose private symbols
// This should fail because secretFunction is private and not imported by wildcard
import * from "wildcard_import_utils.mt";

function main() : void {
    // Try to use a private function - should fail
    int secret = secretFunction();
    print("Secret: " + secret);
}

main();
