// Error: Attempting to throw primitive type (not an exception object)

function testThrowPrimitive(): void {
    try {
        // ERROR: Cannot throw primitive type - must throw an object
        throw 42;
    } catch (int value) {
        print("Caught int: " + value);
    }
}

testThrowPrimitive();
