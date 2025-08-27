// Function parameters cannot be final
function testFunction(final string param): void {
    print(param);
}

print("This should not be reached");