import * from "../../lib/primitives/Int.mt";

// Test cast in generic method call with type inference
class Container<T> {
    T item;

    public function setItem(T newItem): void {
        item = newItem;
    }

    public function getItem(): T {
        return item;
    }
}

// Generic method that accepts any type
function <T> identity(T value): T {
    return value;
}

// Generic method that works with containers
function <T> extractFromContainer(Container<T> container): T {
    return container.getItem();
}

// Generic method with casting
function <T> castAndExtract(Container<?> rawContainer): T {
    Container<T> typedContainer = rawContainer as Container<T>;
    return typedContainer.getItem();
}

// Generic method that creates and returns
function <T> createContainer(T value): Container<T> {
    Container<T> container = new Container<T>();
    container.setItem(value);
    return container;
}

function main(): void {
    // Test identity with type inference
    Int intValue = identity(new Int(42));
    print("Identity result: " + intValue);

    // Test extract from container with inference
    Container<Int> intContainer = new Container<Int>();
    intContainer.setItem(new Int(100));
    Int extracted = extractFromContainer(intContainer);
    print("Extracted: " + extracted);

    // Test cast and extract with explicit type
    Container<?> wildcardContainer = intContainer as Container<?>;
    Int castExtracted = castAndExtract<Int>(wildcardContainer);
    print("Cast extracted: " + castExtracted);

    // Test create container with inference
    Container<Int> newContainer = createContainer(new Int(200));
    Int fromNew = newContainer.getItem();
    print("From new container: " + fromNew);

    // Chain operations with type inference
    Container<Int> chained = createContainer(identity(new Int(300)));
    Int chainedValue = extractFromContainer(chained);
    print("Chained result: " + chainedValue);

    print("Generic method inference cast successful");
}

main();
