// Test: Class cannot be both abstract and final
// Expected error: Class cannot be both abstract and final

// This should cause a parse error
abstract final class InvalidClass {
    abstract function doSomething(): void;
}
