// Null to non-nullable conversion boundary error test
// Tests boundary cases where null cannot be converted to primitive types

function expectsInt(int value): int { 
    return value; 
}

// This should fail: attempting to pass null as primitive int
expectsInt(null);  // null cannot be converted to int