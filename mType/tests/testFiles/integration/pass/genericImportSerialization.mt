import * from "genericLibrary.mt";

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
    print("First int: " + intContainer.getFirst().getValue());
    print("First string: " + stringContainer.getFirst().getValue());

    // Test utility functions (type-specific since no generic functions)
    Bool containsAnswer = containsIntItem(intContainer, new Int(42));
    Bool containsTest = containsStringItem(stringContainer, new String("test"));
    Bool containsMissing = containsStringItem(stringContainer, new String("missing"));

    print("Contains 42: " + (containsAnswer.value ? "true" : "false"));
    print("Contains 'test': " + (containsTest.value ? "true" : "false"));
    print("Contains 'missing': " + (containsMissing.value ? "true" : "false"));

    print("Generic import serialization test completed");
}

main();