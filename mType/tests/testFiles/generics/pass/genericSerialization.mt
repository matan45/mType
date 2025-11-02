import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

// Generic serialization patterns
interface Serializable<T> {
    function serialize(): String;
    function deserialize(String data): T;
}

class SerializableInt extends Serializable<Int> {
    Int value;

    public function SerializableInt(Int v) {
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

class SerializableString extends Serializable<String> {
    String value;

    public function SerializableString(String v) {
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

class Serializer<T> {
    public static function <T> serialize(Serializable<T> obj): String {
        return obj.serialize();
    }
}

function main(): void {
    SerializableInt intObj = new SerializableInt(new Int(123));
    String intSerialized = Serializer.serialize(intObj);
    print(intSerialized);

    SerializableString strObj = new SerializableString(new String("test"));
    String strSerialized = Serializer.serialize(strObj);
    print(strSerialized);
}

main();
