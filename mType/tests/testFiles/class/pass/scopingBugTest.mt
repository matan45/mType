// Test to verify proper lexical scoping (should fail with current bug)
// This test should demonstrate that local variables in functions
// should NOT be accessible from other functions

function testFunction(): void {
    int localVar = 42;
    print("testFunction: localVar = " + localVar);
    helperFunction();  // This should fail - cannot access localVar
}

function helperFunction(): void {
    print("helperFunction: trying to access localVar = " + localVar);  // Should error
}

testFunction();