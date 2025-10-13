// Test: Interface cannot extend class
// Should fail at parse time with clear error message

class BaseClass {
    function getValue(): int {
        return 42;
    }
}

interface MyInterface extends BaseClass {
    function doSomething(): void;
}
