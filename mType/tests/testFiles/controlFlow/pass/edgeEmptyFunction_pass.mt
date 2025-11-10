// Test empty function bodies (minimal valid functions)

// Empty void function
function emptyVoid(): void {
}

// Empty int function with return
function emptyInt(): int {
    return 0;
}

// Empty function that just returns a constant
function justReturn(): int {
    return 42;
}

// Empty function with parameter that's unused
function unusedParam(int x): void {
}

// Multiple empty functions called in sequence
function step1(): void {
}

function step2(): void {
}

function step3(): void {
}

// Call all empty functions
emptyVoid();
print(emptyInt());
print(justReturn());
unusedParam(100);
step1();
step2();
step3();

// Empty function returning result of another empty function
function chainEmpty(): int {
    return emptyInt();
}

print(chainEmpty());

// Empty function in conditional
if (true) {
    emptyVoid();
    print(1);
}

// Empty function in loop
int i = 0;
while (i < 3) {
    emptyVoid();
    i = i + 1;
}
print(i);

print("Test passed"); // Test completed
