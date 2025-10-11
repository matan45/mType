// Test: Import public arrays and objects
// This should succeed because publicNumbers, publicStrings, and publicProcessor are public
// Located in: pass/test_public_array_object_import.mt

import { publicNumbers, publicStrings, publicProcessor } from "models/DataProcessor.mt";

function main() : void {
    print("=== Testing Public Array and Object Imports ===");

    // Access public array
    int firstNumber = publicNumbers[0];
    print("First public number: "+ firstNumber);

    // Access public string array
    string firstString = publicStrings[0];
    print("First public string: " + firstString);

    // Access public object
    int result = publicProcessor.processNumber(5);
    print("Processed number (5^2): " + result);

    print("Public array and object import test completed!");
}

main();
