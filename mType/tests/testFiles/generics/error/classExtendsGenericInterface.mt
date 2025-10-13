// Test: Class cannot extend generic interface
// Should fail at parse time with clear error message

interface Container<T> {
    function get(): T;
}

class MyClass extends Container<string> {
    function get(): string {
        return "hello";
    }
}
