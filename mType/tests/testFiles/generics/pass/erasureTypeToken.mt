import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

// Type token pattern to preserve type information
class TypeToken<T> {
    String typeName;

    public constructor(String name) {
        typeName = name;
    }

    public function getTypeName(): String {
        return typeName;
    }
}

class Container<T> {
    T value;
    TypeToken<T> token;

    public constructor(TypeToken<T> t) {
        token = t;
    }

    public function setValue(T v): void {
        value = v;
    }

    public function getValue(): T {
        return value;
    }

    public function getType(): String {
        return token.getTypeName();
    }
}

function main(): void {
    TypeToken<Int> intToken = new TypeToken<Int>(new String("Integer"));
    Container<Int> intContainer = new Container<Int>(intToken);
    intContainer.setValue(new Int(100));

    print("Type: " + intContainer.getType());
    print("Value: " + intContainer.getValue());
}

main();
