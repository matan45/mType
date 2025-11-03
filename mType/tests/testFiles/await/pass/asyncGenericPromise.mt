// Test promise with generic types

import { Int } from "../../lib/primitives/Int.mt";
import { String } from "../../lib/primitives/String.mt";

print("=== Async Generic Promise Test ===");

class Container<T> {
    T value;

    public constructor(T val) {
        this.value = val;
    }

    public function getValue(): T {
        return this.value;
    }

    public function async <U> transform(U newValue): Promise<Container<U>> {
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

function async createStringContainer(string value): Promise<Container<Stirng>> {
    print("Creating string container: " + value);
    Container<Stirng> container = new Container<Stirng>(new String(value));
    return container;
}

function async main(): Promise<Container<String>> {
    Container<Int> intContainer = await createIntContainer(42);
    Int storedInt = intContainer.getValue();
    print("Stored int: " + storedInt.getValue());

    Container<String> strContainer = await createStringContainer("Hello");
    String storedStr = strContainer.getValue();
    print("Stored string: " + storedStr.getValue());

    // Transform to another type
    Container<String> transformed = await intContainer.transform<String>(new String("Transformed"));
    String transformedVal = transformed.getValue();
    print("Transformed value: " + transformedVal.getValue());

    return transformed;
}

main();
