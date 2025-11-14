// Test that importing private symbols fails
// This should fail because internalHelper is private
import { Calculator, internalHelper } from "selective_import_utils.mt";

function main() : void {
    int result = internalHelper(5);
    print("Result: " + result);
}

main();
