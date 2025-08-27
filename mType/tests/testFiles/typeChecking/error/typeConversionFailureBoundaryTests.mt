// Comprehensive type conversion failure boundary tests
// Tests various boundary conditions and edge cases where type conversions should fail

class TestData {
    int value;
    constructor(int v) { value = v; }
}

class OtherData {
    string name;
    constructor(string n) { name = n; }
}

// Test 1: Complex object to primitive conversion failure
function expectsInt(int x): int { return x; }

TestData obj = new TestData(42);
expectsInt(obj);  // Should fail: object cannot be converted to int