import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

// Type information preservation for reflection-like operations
class Metadata {
    String typeName;
    String description;

    public constructor(String name, String desc) {
        typeName = name;
        description = desc;
    }

    public function getTypeName(): String {
        return typeName;
    }

    public function getDescription(): String {
        return description;
    }
}

class Reflectable<T> {
    T value;
    Metadata meta;

    public constructor(T val, Metadata m) {
        value = val;
        meta = m;
    }

    public function getValue(): T {
        return value;
    }

    public function reflect(): void {
        print("Type: " + meta.getTypeName());
        print("Description: " + meta.getDescription());
        print("Value: " + value);
    }
}

function main(): void {
    Metadata intMeta = new Metadata(new String("Int"), new String("Integer type"));
    Reflectable<Int> intObj = new Reflectable<Int>(new Int(42), intMeta);
    intObj.reflect();
}

main();
