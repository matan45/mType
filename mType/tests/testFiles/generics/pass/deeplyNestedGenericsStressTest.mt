import "../../lib/collections/List.mt";
import "../../lib/collections/HashMap.mt";
import "../../lib/collections/HashSet.mt";
import "../../lib/primitives/String.mt";
import "../../lib/primitives/Int.mt";

// Stress test for deeply nested generic types
function main(): void {
    print("=== Deeply Nested Generics Stress Test ===");

    // Test 1: List<HashMap<String, HashSet<Integer>>>
    print("\nTest 1: List<HashMap<String, HashSet<Int>>>");
    List<HashMap<String, HashSet<Int>>> complexList = new List<HashMap<String, HashSet<Int>>>();

    // Create and populate a HashMap with HashSet values
    HashMap<String, HashSet<Int>> outerMap = new HashMap<String, HashSet<Int>>();

    // Create HashSets with integers
    HashSet<Int> set1 = new HashSet<Int>();
    set1.add(new Int(10));
    set1.add(new Int(20));
    set1.add(new Int(30));

    HashSet<Int> set2 = new HashSet<Int>();
    set2.add(new Int(100));
    set2.add(new Int(200));

    // Add HashSets to HashMap
    outerMap.put(new String("numbers1"), set1);
    outerMap.put(new String("numbers2"), set2);

    // Add HashMap to List
    complexList.add(outerMap);

    // Create second HashMap for the list
    HashMap<String, HashSet<Int>> secondMap = new HashMap<String, HashSet<Int>>();
    HashSet<Int> set3 = new HashSet<Int>();
    set3.add(new Int(500));
    set3.add(new Int(600));
    secondMap.put(new String("numbers3"), set3);

    complexList.add(secondMap);

    print("Complex list size: " + complexList.size());
    print("First map size: " + complexList.get(0).size());
    print("Second map size: " + complexList.get(1).size());

    // Test 2: HashMap<String, List<HashSet<String>>>
    print("\nTest 2: HashMap<String, List<HashSet<String>>>");
    HashMap<String, List<HashSet<String>>> nestedMap = new HashMap<String, List<HashSet<String>>>();

    List<HashSet<String>> stringSetList = new List<HashSet<String>>();

    HashSet<String> stringSet1 = new HashSet<String>();
    stringSet1.add(new String("apple"));
    stringSet1.add(new String("banana"));

    HashSet<String> stringSet2 = new HashSet<String>();
    stringSet2.add(new String("cat"));
    stringSet2.add(new String("dog"));

    stringSetList.add(stringSet1);
    stringSetList.add(stringSet2);

    nestedMap.put(new String("fruits_animals"), stringSetList);

    print("Nested map size: " + nestedMap.size());
    print("String set list size: " + nestedMap.get(new String("fruits_animals")).size());

    // Test 3: Triple nesting - List<List<List<String>>>
    print("\nTest 3: List<List<List<String>>>");
    List<List<List<String>>> tripleList = new List<List<List<String>>>();

    List<List<String>> middleList = new List<List<String>>();
    List<String> innerList1 = new List<String>();
    innerList1.add(new String("deep"));
    innerList1.add(new String("nesting"));

    List<String> innerList2 = new List<String>();
    innerList2.add(new String("stress"));
    innerList2.add(new String("test"));

    middleList.add(innerList1);
    middleList.add(innerList2);
    tripleList.add(middleList);

    print("Triple list size: " + tripleList.size());
    print("Middle list size: " + tripleList.get(0).size());
    print("Inner list 1 size: " + tripleList.get(0).get(0).size());

    // Test 4: HashMap with nested Lists and HashSets
    print("\nTest 4: HashMap<Int, HashMap<String, List<Int>>>");
    HashMap<Int, HashMap<String, List<Int>>> megaMap = new HashMap<Int, HashMap<String, List<Int>>>();

    HashMap<String, List<Int>> subMap = new HashMap<String, List<Int>>();
    List<Int> intList = new List<Int>();
    intList.add(new Int(1000));
    intList.add(new Int(2000));

    subMap.put(new String("large_numbers"), intList);
    megaMap.put(new Int(42), subMap);

    print("Mega map size: " + megaMap.size());
    print("Sub map size: " + megaMap.get(new Int(42)).size());
    print("Int list size: " + megaMap.get(new Int(42)).get(new String("large_numbers")).size());

    // Test 6: Extreme nesting - List<HashMap<String, HashSet<HashMap<Int, String>>>>
    print("\nTest 6: Extreme nesting - List<HashMap<String, HashSet<HashMap<Int, String>>>>");
    List<HashMap<String, HashSet<HashMap<Int, String>>>> extremeNesting = new List<HashMap<String, HashSet<HashMap<Int, String>>>>();

    HashMap<String, HashSet<HashMap<Int, String>>> extremeMap = new HashMap<String, HashSet<HashMap<Int, String>>>();
    HashSet<HashMap<Int, String>> extremeSet = new HashSet<HashMap<Int, String>>();

    HashMap<Int, String> innerMap = new HashMap<Int, String>();
    innerMap.put(new Int(999), new String("extreme"));
    extremeSet.add(innerMap);

    extremeMap.put(new String("deep_key"), extremeSet);
    extremeNesting.add(extremeMap);

    print("Extreme nesting size: " + extremeNesting.size());
    print("Extreme nesting class: " + classNameObj(extremeNesting));

    print("\n=== All deeply nested generic stress tests completed successfully! ===");
}

main();