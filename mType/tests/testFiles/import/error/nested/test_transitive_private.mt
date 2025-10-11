// Level 3: Try to access private symbol from Level 1 through Level 2
// This should fail because internalMathHelper is private in MathCore.mt
// Even though DataProcessor imports from MathCore, private symbols aren't re-exported
// Located in: error/nested/test_transitive_private.mt

import * from "../../pass/models/DataProcessor.mt"

function main() : void {
    // Try to use a private function from the transitive import
    // This should fail - private symbols don't become accessible through re-export
    int result = internalMathHelper(10);
    print("Result: " + string(result));
}

main();
