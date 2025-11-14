import * from "../../lib/collections/ArrayList.mt";
import * from "../../lib/collections/HashMap.mt";
import * from "../../lib/collections/HashSet.mt";
import * from "../../lib/primitives/String.mt";
import * from "../../lib/primitives/Int.mt";

// Stress test for deeply nested generic types
function main(): void {
    print("=== Deeply Nested Generics Stress Test ===");

    // Test 1: ArrayList<HashMap<String, HashSet<Integer>>>
    print("\nTest 1: ArrayList<HashMap<String, HashSet<Int>>>");
    ArrayList<HashMap<String, HashSet<Int>>> complexArrayList = new ArrayList<HashMap<String, HashSet<Int>>>();

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

    // Add HashMap to ArrayList
    complexArrayList.add(outerMap);

    // Create second HashMap for the ArrayList
    HashMap<String, HashSet<Int>> secondMap = new HashMap<String, HashSet<Int>>();
    HashSet<Int> set3 = new HashSet<Int>();
    set3.add(new Int(500));
    set3.add(new Int(600));
    secondMap.put(new String("numbers3"), set3);

    complexArrayList.add(secondMap);

    print("Complex ArrayList size: " + complexArrayList.size());
    print("First map size: " + complexArrayList.get(0).size());
    print("Second map size: " + complexArrayList.get(1).size());

    // Test 2: HashMap<String, ArrayList<HashSet<String>>>
    print("\nTest 2: HashMap<String, ArrayList<HashSet<String>>>");
    HashMap<String, ArrayList<HashSet<String>>> nestedMap = new HashMap<String, ArrayList<HashSet<String>>>();

    ArrayList<HashSet<String>> stringSetArrayList = new ArrayList<HashSet<String>>();

    HashSet<String> stringSet1 = new HashSet<String>();
    stringSet1.add(new String("apple"));
    stringSet1.add(new String("banana"));

    HashSet<String> stringSet2 = new HashSet<String>();
    stringSet2.add(new String("cat"));
    stringSet2.add(new String("dog"));

    stringSetArrayList.add(stringSet1);
    stringSetArrayList.add(stringSet2);

    nestedMap.put(new String("fruits_animals"), stringSetArrayList);

    print("Nested map size: " + nestedMap.size());
    print("String set ArrayList size: " + nestedMap.get(new String("fruits_animals")).size());

    // Test 3: Triple nesting - ArrayList<ArrayList<ArrayList<String>>>
    print("\nTest 3: ArrayList<ArrayList<ArrayList<String>>>");
    ArrayList<ArrayList<ArrayList<String>>> tripleArrayList = new ArrayList<ArrayList<ArrayList<String>>>();

    ArrayList<ArrayList<String>> middleArrayList = new ArrayList<ArrayList<String>>();
    ArrayList<String> innerArrayList1 = new ArrayList<String>();
    innerArrayList1.add(new String("deep"));
    innerArrayList1.add(new String("nesting"));

    ArrayList<String> innerArrayList2 = new ArrayList<String>();
    innerArrayList2.add(new String("stress"));
    innerArrayList2.add(new String("test"));

    middleArrayList.add(innerArrayList1);
    middleArrayList.add(innerArrayList2);
    tripleArrayList.add(middleArrayList);

    print("Triple ArrayList size: " + tripleArrayList.size());
    print("Middle ArrayList size: " + tripleArrayList.get(0).size());
    print("Inner ArrayList 1 size: " + tripleArrayList.get(0).get(0).size());

    // Test 4: HashMap with nested ArrayLists and HashSets
    print("\nTest 4: HashMap<Int, HashMap<String, ArrayList<Int>>>");
    HashMap<Int, HashMap<String, ArrayList<Int>>> megaMap = new HashMap<Int, HashMap<String, ArrayList<Int>>>();

    HashMap<String, ArrayList<Int>> subMap = new HashMap<String, ArrayList<Int>>();
    ArrayList<Int> intArrayList = new ArrayList<Int>();
    intArrayList.add(new Int(1000));
    intArrayList.add(new Int(2000));

    subMap.put(new String("large_numbers"), intArrayList);
    megaMap.put(new Int(42), subMap);

    print("Mega map size: " + megaMap.size());
    print("Sub map size: " + megaMap.get(new Int(42)).size());
    print("Int ArrayList size: " + megaMap.get(new Int(42)).get(new String("large_numbers")).size());

    // Test 6: Extreme nesting - ArrayList<HashMap<String, HashSet<HashMap<Int, String>>>>
    print("\nTest 6: Extreme nesting - ArrayList<HashMap<String, HashSet<HashMap<Int, String>>>>");
    ArrayList<HashMap<String, HashSet<HashMap<Int, String>>>> extremeNesting = new ArrayList<HashMap<String, HashSet<HashMap<Int, String>>>>();

    HashMap<String, HashSet<HashMap<Int, String>>> extremeMap = new HashMap<String, HashSet<HashMap<Int, String>>>();
    HashSet<HashMap<Int, String>> extremeSet = new HashSet<HashMap<Int, String>>();

    HashMap<Int, String> innerMap = new HashMap<Int, String>();
    innerMap.put(new Int(999), new String("extreme"));
    extremeSet.add(innerMap);

    extremeMap.put(new String("deep_key"), extremeSet);
    extremeNesting.add(extremeMap);

    print("Extreme nesting size: " + extremeNesting.size());

    print("\n=== All deeply nested generic stress tests completed successfully! ===");
}

main();