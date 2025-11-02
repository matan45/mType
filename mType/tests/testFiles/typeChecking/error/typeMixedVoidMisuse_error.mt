// Test void type misuse in various contexts
function voidFunction(): void {
    print("This returns void");
}

// Error: Cannot assign void to a variable
int result = voidFunction();

// Error: Cannot use void in expression
int value = voidFunction() + 5;
