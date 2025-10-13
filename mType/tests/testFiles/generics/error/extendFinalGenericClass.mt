// Test: Cannot extend final generic class
// Should fail at parse time with clear error message

final class Container<T> {
    T value;

    function get(): T {
        return value;
    }
}

class MyContainer extends Container<string> {
    function test(): void {
        // This should not be allowed
    }
}
