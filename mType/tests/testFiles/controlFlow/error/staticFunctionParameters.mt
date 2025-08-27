// Function parameters cannot be static
function testFunction(static int param): void {
    print(param);
}

print("This should not be reached");