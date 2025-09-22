import "../lib/collections/HashMap.mt";
import "../lib/collections/HashSet.mt";
import "../lib/primitives/Int.mt";
import "../lib/primitives/String.mt";

// Test serialization with multiple generic collection types
function testMultipleGenerics(): void {
    print("Testing multiple generic types...");

    // Simple arrays with built-in syntax
    int[] intArray = new int[3];
    string[] stringArray = new string[2];

    intArray[0] = 1;
    intArray[1] = 2;
    intArray[2] = 3;

    stringArray[0] = "hello";
    stringArray[1] = "world";

    // HashMap with different key-value combinations
    HashMap<String, Int> stringToInt = new HashMap<String, Int>();
    HashMap<Int, String> intToString = new HashMap<Int, String>();

    stringToInt.put(new String("one"), new Int(1));
    stringToInt.put(new String("two"), new Int(2));

    intToString.put(new Int(10), new String("ten"));
    intToString.put(new Int(20), new String("twenty"));

    // HashSet of different types
    HashSet<String> stringSet = new HashSet<String>();
    HashSet<Int> intSet = new HashSet<Int>();

    stringSet.add(new String("alpha"));
    stringSet.add(new String("beta"));

    intSet.add(new Int(100));
    intSet.add(new Int(200));

    // Hash-based collections
    HashMap<String, Int> hashMap = new HashMap<String, Int>();
    HashSet<String> hashSet = new HashSet<String>();

    hashMap.put(new String("key1"), new Int(100));
    hashMap.put(new String("key2"), new Int(200));

    hashSet.add(new String("hash1"));
    hashSet.add(new String("hash2"));

    // Test all collections
    print("Int array length: " + intArray.length);
    print("String array length: " + stringArray.length);
    print("String->Int map size: " + stringToInt.size());
    print("Int->String map size: " + intToString.size());
    print("String set size: " + stringSet.size());
    print("Int set size: " + intSet.size());
    print("HashMap size: " + hashMap.size());
    print("HashSet size: " + hashSet.size());

    // Test some values
    print("First int: " + intArray[0]);
    print("First string: " + stringArray[0]);
    print("String 'one' maps to: " + stringToInt.get(new String("one")).getValue());
    print("Int 10 maps to: " + intToString.get(new Int(10)).getValue());
    print("HashMap key1: " + hashMap.get(new String("key1")).getValue());
    print("Contains hash1: " + (hashSet.contains(new String("hash1")) ? "true" : "false"));
}

function main(): void {
    testMultipleGenerics();
    print("Multi-type generic serialization test completed");
}

main();