// Test: Interface cannot extend generic class
// Should fail at parse time with clear error message

class Container<T> {
    T value;

    function get(): T {
        return value;
    }
}

interface MyInterface extends Container<string> {
    function doSomething(): void;
}
