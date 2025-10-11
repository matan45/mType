// Level 3: Try to import private symbol from nested chain
// This should fail because InternalProcessor is private in DataProcessor.mt
// Located in: error/nested/test_nested_private_access.mt

import { DataProcessor, InternalProcessor } from "../../pass/models/DataProcessor.mt"

function main() : void {
    DataProcessor dp = new DataProcessor();
    InternalProcessor ip = new InternalProcessor();
    print("Should not reach here");
}

main();
