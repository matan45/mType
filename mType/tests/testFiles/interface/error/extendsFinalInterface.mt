// Error test: Cannot extend a final interface
// This should throw an InheritanceException

final interface FinalInterface {
    function doSomething(): void;
}

// This should fail - attempting to extend a final interface
interface ExtendedInterface extends FinalInterface {
    function doSomethingElse(): void;
}
