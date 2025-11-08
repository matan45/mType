import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

// Test type erasure scenarios - runtime behavior when type info is lost
class Container<T> {
    T value;

    public function setValue(T v): void {
        value = v;
    }

    public function getValue(): T {
        return value;
    }

    public function hasValue(): bool {
        if (value == null) {
            return false;
        }
        return true;
    }
}

class TypeInfo {
    string typeName;

    constructor(string name) {
        typeName = name;
    }

    public function getTypeName(): string {
        return typeName;
    }
}

// Generic function that works regardless of type erasure
function <T> createContainer(T item): Container<T> {
    Container<T> c = new Container<T>();
    c.setValue(item);
    return c;
}

// Test runtime type checking after erasure
function <T> checkAndProcess(Container<T> container): string {
    if (container.hasValue()) {
        return "Container has value: " + container.getValue();
    }
    return "Container is empty";
}

function main(): void {
    print("Testing type erasure scenarios");

    // Create containers with different types
    Container<Int> intContainer = createContainer<Int>(new Int(42));
    Container<String> strContainer = createContainer<String>(new String("Hello"));
    Container<TypeInfo> typeContainer = createContainer<TypeInfo>(new TypeInfo("TestType"));

    // After erasure, containers still work correctly
    print(checkAndProcess<Int>(intContainer));
    print(checkAndProcess<String>(strContainer));
    print(checkAndProcess<TypeInfo>(typeContainer));

    // Test with null values
    Container<Int> nullContainer = new Container<Int>();
    print(checkAndProcess<Int>(nullContainer));

    // Verify type-specific operations still work
    Int intVal = intContainer.getValue();
    print("Int value: " + intVal);

    String strVal = strContainer.getValue();
    print("String value: " + strVal);

    TypeInfo typeVal = typeContainer.getValue();
    print("TypeInfo: " + typeVal.getTypeName());

    print("Type erasure test completed");
}

main();
