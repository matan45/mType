import * from "../../lib/primitives/Int.mt";

// Test unbounded wildcard (?) casting
class Container<T> {
    T item;

    public function setItem(T newItem): void {
        item = newItem;
    }

    public function getItem(): T {
        return item;
    }

    public function hasItem(): bool {
        return item != null;
    }
}

// Function that accepts any Container regardless of type parameter
function printContainerInfo(Container<?> container): void {
    if (container.hasItem()) {
        print("Container has an item");
    } else {
        print("Container is empty");
    }
}

function main(): void {
    // Create containers with different types
    Container<Int> intContainer = new Container<Int>();
    intContainer.setItem(new Int(42));

    Container<String> stringContainer = new Container<String>();
    stringContainer.setItem("Hello");

    // Cast to unbounded wildcard
    Container<?> wildcardContainer1 = intContainer as Container<?>;
    Container<?> wildcardContainer2 = stringContainer as Container<?>;

    // Use wildcards with type-agnostic operations
    printContainerInfo(wildcardContainer1);
    printContainerInfo(wildcardContainer2);

    // Direct usage
    if (wildcardContainer1.hasItem()) {
        print("First container check passed");
    }

    if (wildcardContainer2.hasItem()) {
        print("Second container check passed");
    }

    print("Wildcard cast successful");
}

main();
