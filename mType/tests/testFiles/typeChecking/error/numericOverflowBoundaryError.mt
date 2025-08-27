// Numeric overflow boundary error test
// Tests boundary cases with float to int conversion failures

function expectsInt(int x): int { 
    return x; 
}

// This should fail: attempting to pass float as int without explicit conversion
expectsInt(3.14);  // float cannot be implicitly converted to int