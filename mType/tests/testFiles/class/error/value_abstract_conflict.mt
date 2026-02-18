// Test: Value class cannot be abstract
// Expected error: Class cannot be both value and abstract

value abstract class InvalidValueClass {
    abstract function doSomething(): void;
}
