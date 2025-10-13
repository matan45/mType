// Test: Cannot extend final interface
// Should fail at parse time with clear error message

final interface BaseInterface {
    function doSomething(): void;
}

interface ChildInterface extends BaseInterface {
    function doMore(): void;
}
