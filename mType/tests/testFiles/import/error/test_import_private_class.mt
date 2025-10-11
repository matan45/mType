// Test that importing private classes fails
// This should fail because InternalCache is private
import { Calculator, InternalCache } from "selective_import_utils.mt"

function main() : void {
    InternalCache cache = new InternalCache();
    print("Cache created");
}

main();
