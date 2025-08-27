// Static modifier cannot be applied to namespace declarations
static namespace invalid {
    int value = 10;
    
    function getValue(): int {
        return value;
    }
}

// This code should never be reached due to parse error
print("This should not print");