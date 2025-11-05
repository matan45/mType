// Comprehensive test: Import interfaces with extends and multiple generic types
import * from "BaseInterfaces.mt";
import * from "ExtendedInterfaces.mt";
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";
import * from "../../lib/primitives/Bool.mt";

// Test class implementing complex interface hierarchy
class StringIntCache implements CacheStore<String, Int, String> {
    // From Repository<Int, String> (inherited)
    public function save(Int entity): Int {
        return entity;
    }

    public function findById(String id): Int {
        return new Int(42);
    }

    public function delete(String id): void {
        print("Deleting " + id);
    }

    public function equals(Repository<Int, String> other): bool {
        return false;
    }

    // From Hashable (inherited)
    public function hashCode(): int {
        return 123;
    }

    // From CacheStore<String, Int, String>
    public function cache(String key, Int value, String metadata): void {
        print("Caching " + key + " -> " + value + " [" + metadata + "]");
    }

    public function evict(String key): bool {
        print("Evicting " + key);
        return true;
    }

    public function getMetadata(String key): String {
        return new String("metadata-" + key);
    }
}

// Test class for SortableMap
class StringIntMap implements SortableMap<String, Int> {
    public function put(String key, Int value): void {
        print("Put " + key + " -> " + value);
    }

    public function get(String key): Int {
        return new Int(100);
    }

    public function size(): int {
        return 5;
    }

    public function compareTo(String other): int {
        return this.size() - 3; // Simple comparison
    }
}

print("=== Complex Interface Import, Inheritance & Generics Test ===");

// Test 1: Multiple inheritance with complex generics
StringIntCache cache = new StringIntCache();
print("\n--- Testing CacheStore (multiple inheritance) ---");
cache.cache(new String("user1"), new Int(100), new String("active"));
cache.evict(new String("user2"));
print("Metadata: " + cache.getMetadata(new String("user1")));
print("Hash code: " + cache.hashCode());

// Test 2: Single inheritance with generics
StringIntMap map = new StringIntMap();
print("\n--- Testing SortableMap (single inheritance) ---");
map.put(new String("test"), new Int(42));
print("Size: " + map.size());
print("Value: " + map.get(new String("test")));

print("Comparison result: " + map.compareTo(new String("test")));

// Test 3: Lambda expressions with complex generic interfaces
print("\n--- Testing Lambda with Multiple Generic Types ---");

// BiFunction with three different types
BiFunction<String, Int, Bool> stringIntToBool = (str, num) -> {
    return str.length() > num.getValue();
};
print("String length > 5: " + stringIntToBool.apply(new String("hello"), new Int(5)));
print("String length > 1: " + stringIntToBool.apply(new String("hi"), new Int(1)));

// BiPredicate with two generic types
BiPredicate<String, String> stringEquals = (s1, s2) -> {
    return s1.equals(s2);
};
print("Strings equal: " + stringEquals.test(new String("test"), new String("test")));
print("Strings equal: " + stringEquals.test(new String("test"), new String("other")));

// Test 4: Complex type assignments
print("\n--- Testing Complex Type Assignments ---");

// Assigning to base interface types
Repository<Int, String> repo = cache; // CacheStore extends Repository
print("Repository save: " + repo.save(999));

// Test hashable interface (inherited from Repository)
Hashable hashable = cache;
print("Hash code: " + hashable.hashCode());

print("Repository equals check: " + repo.equals(repo));

// Test 5: Multiple generic interface usage
print("\n--- Testing Multiple Generic Interface Usage ---");
BiFunction<Int, Int, Bool> intComparison = (a, b) -> {
    return a.getValue() > b.getValue();
};
print("10 > 5: " + intComparison.apply(new Int(10), new Int(5)));

BiPredicate<Int, Int> intPredicate = (x, y) -> {
    return x.getValue() == y.getValue();
};
print("5 == 5: " + intPredicate.test(new Int(5), new Int(5)));

print("\n=== All Tests Completed Successfully ===");
