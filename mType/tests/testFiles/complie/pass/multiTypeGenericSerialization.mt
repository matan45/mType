// Test multiple type parameter generic serialization/deserialization
class KeyValuePair<K, V> {
    K key;
    V value;

    function setKey(K newKey): void {
        key = newKey;
    }

    function setValue(V newValue): void {
        value = newValue;
    }

    function getKey(): K {
        return key;
    }

    function getValue(): V {
        return value;
    }

    function toString(): string {
        return key + " -> " + value;
    }
}

function main(): void {
    KeyValuePair<string, int> stringIntPair = new KeyValuePair<string, int>();
    stringIntPair.setKey("count");
    stringIntPair.setValue(100);
    print("String-Int: " + stringIntPair.toString());

    KeyValuePair<int, string> intStringPair = new KeyValuePair<int, string>();
    intStringPair.setKey(1);
    intStringPair.setValue("first");
    print("Int-String: " + intStringPair.toString());

    KeyValuePair<bool, float> boolFloatPair = new KeyValuePair<bool, float>();
    boolFloatPair.setKey(true);
    boolFloatPair.setValue(3.14);
    print("Bool-Float: " + boolFloatPair.toString());
}

main();