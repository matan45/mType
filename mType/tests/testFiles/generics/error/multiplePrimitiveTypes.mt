// Test that multiple primitive types are rejected in generics
// This should fail with "Invalid type arguments" error

class KeyValuePair<K, V> {
    K key;
    V value;

    constructor(K k, V v) {
        key = k;
        value = v;
    }

    function getKey(): K {
        return key;
    }

    function getValue(): V {
        return value;
    }
}

function main(): void {
    // This should fail - both string and int are primitive types
    KeyValuePair<string, int> pair = new KeyValuePair<string, int>("test", 42);
    print(pair.getKey());
    print(pair.getValue());
}

main();