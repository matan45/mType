// Array to scalar conversion boundary error test
// Tests boundary cases where array types cannot be converted to scalar types

function expectsString(string text): string { 
    return text; 
}

class Container {
    string data;
    constructor(string d) { data = d; }
}

Container items = new Container("test");

// This should fail: attempting to pass object as string
expectsString(items);  // Container object cannot be converted to string