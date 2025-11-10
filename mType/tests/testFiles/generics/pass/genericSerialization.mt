import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

// Generic serialization patterns
interface Serializable<T> {
    function serialize(): String;
    function deserialize(String data): T;
}

class SerializableInt implements Serializable<Int> {
    Int value;

    public constructor(Int v) {
        value = v;
    }

    public function serialize(): String {
        return new String("Int:" + value);
    }

    public function deserialize(String data): Int {
        return new Int(42);
    }

    public function getValue(): Int {
        return value;
    }
}

class SerializableString implements Serializable<String> {
    String value;

    public constructor(String v) {
        value = v;
    }

    public function serialize(): String {
        return new String("String:" + value);
    }

    public function deserialize(String data): String {
        return new String("Deserialized");
    }

    public function getValue(): String {
        return value;
    }
}

class Serializer {
    public static function <U> serialize(Serializable<U> obj): String {
        return obj.serialize();
    }
}

function main(): void {
    SerializableInt intObj = new SerializableInt(new Int(123));
    String intSerialized = Serializer::serialize<Int>(intObj);
    print(intSerialized.toString());

    SerializableString strObj = new SerializableString(new String("test"));
    String strSerialized = Serializer::serialize<String>(strObj);
    print(strSerialized.toString());
}

main();
