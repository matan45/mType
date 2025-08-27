// Function type conversion boundary error test
// Tests boundary cases where functions cannot be used as values

function testFunction(): int {
    return 42;
}

function expectsInt(int value): int { 
    return value; 
}

// This should fail: attempting to pass function reference as int
expectsInt(testFunction);  // function cannot be converted to int