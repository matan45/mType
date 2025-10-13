// Test: Cannot extend final generic interface
// Should fail at parse time with clear error message

final interface Container<T> {
    function get(): T;
}

interface MyInterface extends Container<string> {
    function doMore(): void;
}
