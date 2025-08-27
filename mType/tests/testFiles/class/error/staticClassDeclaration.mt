// Static modifier cannot be applied to class declarations
static class InvalidClass {
    int value;
    
    constructor() {
        value = 10;
    }
}

// This code should never be reached due to parse error
print("This should not print");