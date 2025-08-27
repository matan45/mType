int globalCounter = 0;

function incrementCounter(): void {
    globalCounter = globalCounter + 1;
}

function printCounter(): void {
    print(globalCounter);
}

function resetCounter(): void {
    globalCounter = 0;
}

printCounter();  // 0
incrementCounter();
incrementCounter();
incrementCounter();
printCounter();  // 3
resetCounter();
printCounter();  // 0
print("Test passed"); // Test completed