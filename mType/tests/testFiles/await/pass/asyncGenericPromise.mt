// Test promise with generic types

import { Int } from "../../lib/primitives/Int.mt";

print("=== Async Generic Promise Test ===");

class Container<T> {
    T value;

    public constructor(T val) {
        this.value = val;
    }

    public function getValue(): T {
        return this.value;
    }

    public function async transform<U>(U newValue): Promise<Container<U>> {
        print("Transforming container");
        Container<U> newContainer = new Container<U>(newValue);
        return newContainer;
    }
}

function async createIntContainer(int value): Promise<Container<Int>> {
    print("Creating Int container: " + value);
    Int intVal = new Int(value);
    Container<Int> container = new Container<Int>(intVal);
    return container;
}

function async createStringContainer(string value): Promise<Container<string>> {
    print("Creating string container: " + value);
    Container<string> container = new Container<string>(value);
    return container;
}

function async main(): Promise<Container<string>> {
    Container<Int> intContainer = await createIntContainer(42);
    Int storedInt = intContainer.getValue();
    print("Stored int: " + storedInt);

    Container<string> strContainer = await createStringContainer("Hello");
    string storedStr = strContainer.getValue();
    print("Stored string: " + storedStr);

    // Transform to another type
    Container<string> transformed = await intContainer.transform<string>("Transformed");
    string transformedVal = transformed.getValue();
    print("Transformed value: " + transformedVal);

    return transformed;
}

main();
