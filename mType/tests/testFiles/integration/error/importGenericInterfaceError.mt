// Test: Import with type mismatch - should error
// @Throw

import "modules/StrictInterface.mt";

class WrongProcessor implements StrictProcessor<String> {
    // ERROR: Wrong return type - returns Int instead of String
    process(input: String) : Int {
        return input.length();
    }
}

main() : Void {
    let processor = new WrongProcessor();
    let result = processor.process("test");
}
