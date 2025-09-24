// Test: Function Scoping - Fail Case
// Functions should NOT access local variables from other functions (lexical scoping)

function callerFunction(): void {
    int localVar = 42;
    print("callerFunction: localVar = " + localVar);
    calledFunction(); // This should fail when calledFunction tries to access localVar
}

function calledFunction(): void {
    print("calledFunction: trying to access localVar = " + localVar); // Should error
}

callerFunction();