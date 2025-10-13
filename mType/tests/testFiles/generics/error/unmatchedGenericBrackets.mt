// Test: Unmatched generic brackets
// Should fail at parse time with clear error message

import * from "../../lib/collections/List.mt";

class Container<T> {
    // This will cause a tokenizer error, but let's test what we can
    T data;
}

function main(): void {
    // Try to create with unmatched brackets
    Container<String c = new Container<String>();
}
