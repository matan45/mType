// Test: Try to import private arrays and objects
// This should fail because privateNumbers, privateData, and privateProcessor are private
// Located in: error/nested/test_private_array_object_access.mt

import { publicNumbers, privateNumbers, publicProcessor, privateProcessor } from "../../pass/models/DataProcessor.mt";

function main() : void {
    print("Should not reach here - private symbols imported");
}

main();
