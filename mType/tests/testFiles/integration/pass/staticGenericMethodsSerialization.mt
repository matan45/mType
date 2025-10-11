import * from "../../lib/collections/HashMap.mt";
import * from "../../lib/collections/HashSet.mt";
import * from "../../lib/collections/LinkedList.mt";
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

// Test serialization with static generic methods and collections
class DataProcessor {
    // Static generic method that returns a collection
    public static function <T> createCollection(T item1, T item2, T item3): HashSet<T> {
        HashSet<T> collection = new HashSet<T>();
        collection.add(item1);
        collection.add(item2);
        collection.add(item3);
        return collection;
    }

    // Static generic method that processes a map
    public static function <K, V> processMap(HashMap<K, V> map, K key, V value): HashMap<K, V> {
        HashMap<K, V> result = new HashMap<K, V>();
        // Copy existing entries
        K[] keys = map.getKeys();
        for (K existingKey : keys) {
            if (existingKey != null) {
                result.put(existingKey, map.get(existingKey));
            }
        }
        // Add new entry
        result.put(key, value);
        return result;
    }

    // Static generic method with multiple type parameters
    public static function <T, U> createMixedCollection(T item1, U item2): LinkedList<String> {
        LinkedList<String> result = new LinkedList<String>();
        result.add(new String("Type1: " + item1.toString()));
        result.add(new String("Type2: " + item2.toString()));
        return result;
    }

    // Static generic method that returns count
    public static function <T> countUniqueItems(HashSet<T> set, T newItem): int {
        int originalSize = set.size();
        set.add(newItem);
        int newSize = set.size();
        return newSize - originalSize; // 1 if new, 0 if duplicate
    }

    // Static generic method for data transformation
    public static function <T> transformToMap(HashSet<T> set): HashMap<String, T> {
        HashMap<String, T> result = new HashMap<String, T>();
        int index = 0;
        T[] items = set.toArray();
        for (T item : items) {
            if (item != null) {
                result.put(new String("item_" + index), item);
                index++;
            }
        }
        return result;
    }
}

function main(): void {
    print("Testing static generic methods with serialization...");

    // Test 1: Create collections with different types
    HashSet<String> stringSet = DataProcessor::createCollection<String>(
        new String("alpha"),
        new String("beta"),
        new String("gamma")
    );
    print("String set size: " + stringSet.size());
    print("Contains alpha: " + (stringSet.contains(new String("alpha")) ? "true" : "false"));

    HashSet<Int> intSet = DataProcessor::createCollection<Int>(
        new Int(10),
        new Int(20),
        new Int(30)
    );
    print("Int set size: " + intSet.size());
    print("Contains 20: " + (intSet.contains(new Int(20)) ? "true" : "false"));

    // Test 2: Process maps with static generic method
    HashMap<String, Int> originalMap = new HashMap<String, Int>();
    originalMap.put(new String("first"), new Int(100));
    originalMap.put(new String("second"), new Int(200));

    HashMap<String, Int> processedMap = DataProcessor::processMap<String, Int>(
        originalMap,
        new String("third"),
        new Int(300)
    );
    print("Processed map size: " + processedMap.size());
    print("Contains third: " + processedMap.containsKey(new String("third")));
    print("Third value: " + processedMap.get(new String("third")).getValue());

    // Test 3: Mixed type collections
    LinkedList<String> mixedList = DataProcessor::createMixedCollection<String, Int>(
        new String("text"),
        new Int(42)
    );
    print("Mixed list size: " + mixedList.size());
    print("First item: " + mixedList.get(0).getValue());
    print("Second item: " + mixedList.get(1).getValue());

    // Test 4: Count unique items
    HashSet<String> testSet = new HashSet<String>();
    testSet.add(new String("existing"));

    int added1 = DataProcessor::countUniqueItems<String>(testSet, new String("new"));
    print("Added new item (should be 1): " + added1);

    int added2 = DataProcessor::countUniqueItems<String>(testSet, new String("existing"));
    print("Added duplicate item (should be 0): " + added2);

    // Test 5: Transform set to map
    HashSet<Int> numberSet = new HashSet<Int>();
    numberSet.add(new Int(5));
    numberSet.add(new Int(15));
    numberSet.add(new Int(25));

    HashMap<String, Int> transformedMap = DataProcessor::transformToMap<Int>(numberSet);
    print("Transformed map size: " + transformedMap.size());
    print("Contains item_0: " + transformedMap.containsKey(new String("item_0")));

    // Test 6: Chaining static generic method calls
    HashSet<String> chainedSet = DataProcessor::createCollection<String>(
        new String("one"),
        new String("two"),
        new String("three")
    );
    HashMap<String, String> chainedMap = DataProcessor::transformToMap<String>(chainedSet);
    print("Chained operation result size: " + chainedMap.size());

    print("Static generic methods serialization test completed successfully");
}

main();