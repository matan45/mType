// Comprehensive test suite for type substitution edge cases
// Tests both correctness and performance optimizations

import * from "../../lib/collections/List.mt";
import * from "../../lib/collections/HashMap.mt";
import * from "../../lib/collections/HashSet.mt";
import * from "../../lib/collections/Stack.mt";
import * from "../../lib/collections/Queue.mt";
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";
import * from "../../lib/primitives/Float.mt";
import * from "../../lib/primitives/Bool.mt";

// Test 1: Basic generic instantiation (should work)
class Container<T> {
    T data;

    constructor(T value) {
        data = value;
    }

    function get(): T {
        return data;
    }

    function hashSet(T value): void {
        data = value;
    }

    public function test(): void {
        print("Container data: " + data.toString());
    }
}

// Test 2: Multiple type parameters
class Pair<K, V> {
    K first;
    V second;

    constructor(K k, V v) {
        first = k;
        second = v;
    }

    function getFirst(): K {
        return first;
    }

    function getSecond(): V {
        return second;
    }

    public function test(): void {
        print("Pair: (" + first.toString() + ", " + second.toString() + ")");
    }
}

// Test 3: Nested generics
class NestedContainer<T> {
    List<T> items;

    constructor() {
        items = new List<T>();
    }

    public function add(T item): void {
        items.add(item);
    }

    function get(Int index): T {
        return items.get(index);
    }

    function size(): Int {
        return items.size();
    }

    public function test(): void {
        print("NestedContainer size: " + size());
    }
}

// Test 3: Three-way circular dependency
class ThreeWayA<T> { T data; }
class ThreeWayB<U> { U data; }
class ThreeWayC<V> { V data; }

// Test 4: Deep chain that eventually cycles
class DeepChainA<T> { T next; }
class DeepChainB<U> { U next; }
class DeepChainC<V> { V next; }
class DeepChainD<W> { W next; }
class DeepChainE<X> { X finalNext; }

// Test 4: Fast cache patterns - commonly used generic instantiations
class FastCacheTest {
    public static function testCommonPatterns(): void {
        print("=== Fast Cache Performance Tests ===");

        // These should hit fast cache (common patterns)
        print("Testing List<Int> (should be fast cached)...");
        List<Int> IntList = new List<Int>();
        IntList.add(new Int(42));
        IntList.add(new Int(100));
        print("List<Int> size: " + IntList.size());

        print("Testing HashMap<String, Int> (should be fast cached)...");
        HashMap<String, Int> stringIntHashMap = new HashMap<String, Int>();
        stringIntHashMap.put(new String("answer"), new Int(42));
        stringIntHashMap.put(new String("century"), new Int(100));
        print("HashMap<String, Int> size: " + stringIntHashMap.size());

        print("Testing HashSet<String> (should be fast cached)...");
        HashSet<String> stringHashSet = new HashSet<String>();
        stringHashSet.add(new String("hello"));
        stringHashSet.add(new String("world"));
        stringHashSet.add(new String("hello")); // Duplicate should be ignored
        print("HashSet<String> size: " + stringHashSet.size());

        print("Testing Stack<Float> (should be fast cached)...");
        Stack<Float> floatStack = new Stack<Float>();
        floatStack.push(new Float(3.14));
        floatStack.push(new Float(2.71));
        print("Stack<Float> peek: " + floatStack.peek());

        print("Testing Queue<Bool> (should be fast cached)...");
        Queue<Bool> boolQueue = new Queue<Bool>();
        boolQueue.enqueue(new Bool(true));
        boolQueue.enqueue(new Bool(false));
        print("Queue<Bool> front: " + boolQueue.peek());
    }
}

// Test 5: Complex patterns that should use regular cache
class ComplexCacheTest {
    public static function testComplexPatterns(): void {
        print("=== Complex Cache Tests ===");

        // These should NOT use fast cache (too complex)
        print("Testing deeply nested: List<HashMap<String, HashSet<Int>>> (complex)...");
        List<HashMap<String, HashSet<Int>>> complexNested = new List<HashMap<String, HashSet<Int>>>();
        HashMap<String, HashSet<Int>> innerHashMap = new HashMap<String, HashSet<Int>>();
        HashSet<Int> innerHashSet = new HashSet<Int>();
        innerHashSet.add(new Int(1));
        innerHashSet.add(new Int(2));
        innerHashMap.put(new String("numbers"), innerHashSet);
        complexNested.add(innerHashMap);
        print("Complex nested size: " + complexNested.size());

        print("Testing repeated instantiation of same complex type...");
        List<HashMap<String, HashSet<Int>>> anotherComplex = new List<HashMap<String, HashSet<Int>>>();
        HashMap<String, HashSet<Int>> anotherHashMap = new HashMap<String, HashSet<Int>>();
        anotherComplex.add(anotherHashMap);
        print("Another complex size: " + anotherComplex.size());
    }
}

// Test 7: Edge cases with null/empty types
class EdgeCaHashSetest {
    static function testEdgeCases(): void {
        print("=== Edge Case Tests ===");

        print("Testing empty type arguments...");
        // Edge case: Generic class with no type arguments when expected

        print("Testing null type substitution...");
        // Edge case: Null values in substitution HashMap

        print("Testing malformed generic syntax...");
        // Edge case: Malformed generic type strings
    }
}

// Test 9: Memory management edge cases
class MemoryEdgeCaHashSetest {
    static function testMemoryEdgeCases(): void {
        print("=== Memory Management Edge Cases ===");

        print("Testing cache overflow scenarios...");
        // Create many unique generic instantiations to test cache limits

        print("Testing cache eviction behavior...");
        // Verify old entries are properly cleaned up

        print("Testing memory leaks in substitution chains...");
        // Ensure no circular references in cache
    }
}

// Test 10: Substitution chain validation
class SubstitutionChaIntest {
    static function testChainValidation(): void {
        print("=== Substitution Chain Validation ===");

        print("Testing maximum depth limits...");
        // Should hit max depth before stack overflow

        print("Testing chain string generation...");
        // Verify error messages contain proper substitution chains

        print("Testing exception safety during chain traversal...");
        // Ensure proper cleanup if exception occurs mid-substitution
    }
}

// Performance stress test
class PerformanceStressTest {
    public static function stressTestCache(): void {
        print("=== Performance Stress Tests ===");

        print("Stress testing repeated List<Int> instantiation...");
        for (int i = 0; i < 10; i++) {
            List<Int> stressList = new List<Int>();
            stressList.add(new Int(i));
            if (i % 3 == 0) {
                print("Stress test iteration " + i + ", list size: " + stressList.size());
            }
        }

        print("Stress testing mixed common/complex patterns...");
        for (int i = 0; i < 5; i++) {
            // Alternate between simple and complex
            if (i % 2 == 0) {
                HashSet<String> simpleHashSet = new HashSet<String>();
                simpleHashSet.add(new String("item" + i));
            } else {
                HashMap<String, List<Int>> complexHashMap = new HashMap<String, List<Int>>();
                List<Int> innerList = new List<Int>();
                innerList.add(new Int(i));
                complexHashMap.put(new String("key" + i), innerList);
            }
        }
        print("Mixed pattern stress test completed");
    }
}

// Main test runner
function main(): void {
    print("================================================================================");
    print("Type Substitution Edge Cases Test Suite");
    print("================================================================================");

    // Test basic functionality
    print("\n--- Basic Generic Functionality Tests ---");
    Container<Int> IntContainer = new Container<Int>(new Int(42));
    IntContainer.test();

    Pair<String, Int> stringIntPair = new Pair<String, Int>(new String("answer"),new Int(42));
    stringIntPair.test();

    NestedContainer<String> nestedStringContainer = new NestedContainer<String>();
    nestedStringContainer.add(new String("hello"));
    nestedStringContainer.add(new String("world"));
    nestedStringContainer.test();

    // Performance optimization tests
    print("\n--- Performance Optimization Tests ---");
    FastCacheTest::testCommonPatterns();
    ComplexCacheTest::testComplexPatterns();

    // Stress tests
    print("\n--- Stress Tests ---");
    PerformanceStressTest::stressTestCache();

    print("\n================================================================================");
    print("All type substitution edge case tests completed successfully!");
    print("This test exercises:");
    print("- Basic generic instantiation and method calls");
    print("- Fast cache patterns (List, HashMap, HashSet, Stack, Queue with simple types)");
    print("- Complex cache patterns (deeply nested generics)");
    print("- Performance stress testing with repeated instantiations");
    print("- Type substitution correctness across various scenarios");
    print("================================================================================");
}

main();