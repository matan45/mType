import "genericLibrary.mt";

// Test serialization with generic imports
function main(): void {
    print("Testing generic import serialization...");

    // Use imported utility functions (no generic functions in mType)
    GenericContainer<Int> intContainer = createIntContainer();
    GenericContainer<String> stringContainer = createStringContainer();

    // Add items
    intContainer.add(new Int(42));
    intContainer.add(new Int(84));

    stringContainer.add(new String("test"));
    stringContainer.add(new String("import"));

    // Test the containers
    print("Int container size: " + intContainer.size());
    print("String container size: " + stringContainer.size());
    print("First int: " + intContainer.get(0).getValue());
    print("First string: " + stringContainer.get(0).getValue());

    // Test utility functions (type-specific since no generic functions)
    bool containsAnswer = containsIntItem(intContainer, new Int(42));
    bool containsTest = containsStringItem(stringContainer, new String("test"));
    bool containsMissing = containsStringItem(stringContainer, new String("missing"));

    print("Contains 42: " + (containsAnswer ? "true" : "false"));
    print("Contains 'test': " + (containsTest ? "true" : "false"));
    print("Contains 'missing': " + (containsMissing ? "true" : "false"));

    print("Generic import serialization test completed");
}

main();