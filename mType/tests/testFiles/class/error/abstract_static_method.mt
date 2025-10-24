// Test: Static methods cannot be abstract
// Expected error: Method cannot be both abstract and static

class InvalidClass {
    // This should cause a parse error
    abstract static function doSomething(): void;
}
