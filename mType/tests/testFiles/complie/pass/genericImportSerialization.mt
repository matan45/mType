// Test generic import serialization/deserialization
import "genericLibrary.mt";

function main(): void {
    // Test SimpleContainer with different types
    SimpleContainer<int> intContainer = new SimpleContainer<int>();
    intContainer.store(42);
    print("Int container empty: " + intContainer.isEmpty());
    print("Int container value: " + intContainer.retrieve());

    SimpleContainer<string> stringContainer = new SimpleContainer<string>();
    stringContainer.store("hello");
    print("String container empty: " + stringContainer.isEmpty());
    print("String container value: " + stringContainer.retrieve());

    // Test Validator with different types
    Validator<int> intValidator = new Validator<int>();
    print(intValidator.validateAndProcess(100));

    Validator<string> stringValidator = new Validator<string>();
    print(stringValidator.validateAndProcess("test"));

    // Test combined usage
    SimpleContainer<bool> boolContainer = new SimpleContainer<bool>();
    Validator<bool> boolValidator = new Validator<bool>();

    boolContainer.store(true);
    bool value = boolContainer.retrieve();
    print(boolValidator.validateAndProcess(value));
}

main();