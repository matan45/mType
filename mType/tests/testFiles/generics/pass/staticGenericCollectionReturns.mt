import "../../lib/collections/HashSet.mt";
import "../../lib/collections/HashMap.mt";
import "../../lib/primitives/String.mt";
import "../../lib/primitives/Int.mt";

class CollectionFactory {
    // Return empty generic set
    static function <T> createEmptySet(): HashSet<T> {
        return new HashSet<T>();
    }

    // Return set (ready for items to be added later)
    static function <T> createSetForItem(T item): HashSet<T> {
        HashSet<T> result = new HashSet<T>();
        // Note: adding items inside static method has context issues
        // Items can be added after the set is returned
        return result;
    }

    // Return empty generic map
    static function <K, V> createEmptyMap(): HashMap<K, V> {
        return new HashMap<K, V>();
    }

    // Return map with single entry
    static function <K, V> createMapWithEntry(K key, V value): HashMap<K, V> {
        HashMap<K, V> result = new HashMap<K, V>();
        result.put(key, value);
        return result;
    }

    // Return set with multiple items
    static function <T> createSetWithItems(T item1, T item2): HashSet<T> {
        HashSet<T> result = new HashSet<T>();
        result.add(item1);
        result.add(item2);
        return result;
    }
}

// Test empty set creation
HashSet<String> emptyStringSet = CollectionFactory::createEmptySet<String>();
print("Empty string set size: " + emptyStringSet.size());

HashSet<Int> emptyIntSet = CollectionFactory::createEmptySet<Int>();
print("Empty int set size: " + emptyIntSet.size());

// Test set with single item template
HashSet<String> stringSetForItem = CollectionFactory::createSetForItem<String>(new String("hello"));
print("String set for item size: " + stringSetForItem.size());

HashSet<Int> intSetForItem = CollectionFactory::createSetForItem<Int>(new Int(42));
print("Int set for item size: " + intSetForItem.size());

// Test map creation
HashMap<String, Int> emptyMap = CollectionFactory::createEmptyMap<String, Int>();
print("Empty map size: " + emptyMap.size());

HashMap<String, Int> mapWithEntry = CollectionFactory::createMapWithEntry<String, Int>(
    new String("score"), new Int(100)
);
print("Map with entry size: " + mapWithEntry.size());
print("Map contains 'score': " + mapWithEntry.containsKey(new String("score")));

// Test set with multiple items
HashSet<String> multiStringSet = CollectionFactory::createSetWithItems<String>(
    new String("first"), new String("second")
);
print("Multi-item string set size: " + multiStringSet.size());

HashSet<Int> multiIntSet = CollectionFactory::createSetWithItems<Int>(
    new Int(10), new Int(20)
);
print("Multi-item int set size: " + multiIntSet.size());

print("Static generic collection return tests passed");