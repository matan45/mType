// Boolean conversion boundary error test
// Tests boundary cases where boolean conversions should fail

function expectsBool(bool flag): bool { 
    return flag; 
}

// This should fail: attempting to pass string as boolean
expectsBool("true");  // string "true" cannot be converted to boolean