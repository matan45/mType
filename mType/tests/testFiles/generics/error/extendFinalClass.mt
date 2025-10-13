// Test: Cannot extend final class
// Should fail at parse time with clear error message

final class BaseClass {
    int value;

    function getValue(): int {
        return value;
    }
}

class ChildClass extends BaseClass {
    function test(): void {
        // This should not be allowed
    }
}
