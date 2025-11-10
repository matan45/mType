// Test that you can only catch Exception types
class NotAnException {
    public int value;
}

function testCatch(): void {
    try {
        print("In try block");
    } catch (NotAnException e) {  // ERROR: NotAnException doesn't inherit from Exception
        print("Should not compile");
    }
}

testCatch();
