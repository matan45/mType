import * from "../../lib/collections/List.mt";
import * from "../../lib/collections/HashMap.mt";
import * from "../../lib/collections/HashSet.mt";
import * from "../../lib/primitives/String.mt";
import * from "../../lib/primitives/Int.mt";

// Custom generic classes for stress testing
class Container<T, U, V> {
    T first;
    U second;
    V third;

    public constructor(T f, U s, V t) {
        first = f;
        second = s;
        third = t;
    }

    public function getFirst(): T {
        return first;
    }

    public function getSecond(): U {
        return second;
    }

    public function getThird(): V {
        return third;
    }

    public function hashCode(): int {
        int hash = 7;
        hash = hash * 31 + first.hashCode();
        hash = hash * 31 + second.hashCode();
        hash = hash * 31 + third.hashCode();
        return hash;
    }

    public function equals(Container<T, U, V> other): bool {
        if (other == null) return false;
        return first.equals(other.first) &&
               second.equals(other.second) &&
               third.equals(other.third);
    }
}

class Wrapper<T> {
    T data;

    public constructor(T d) {
        data = d;
    }

    public function getData(): T {
        return data;
    }

    public function hashCode(): int {
        return data.hashCode();
    }

    public function equals(Wrapper<T> other): bool {
        if (other == null) return false;
        return data.equals(other.data);
    }
}

function main(): void {
    print("=== Generic Complexity Stress Test ===");

    // Test 1: Multiple type parameters with collections
    print("\nTest 1: Container<List<String>, HashMap<Int, String>, HashSet<Int>>");

    List<String> stringList = new List<String>();
    stringList.add(new String("test1"));
    stringList.add(new String("test2"));

    HashMap<Int, String> intStringMap = new HashMap<Int, String>();
    intStringMap.put(new Int(1), new String("one"));
    intStringMap.put(new Int(2), new String("two"));

    HashSet<Int> intSet = new HashSet<Int>();
    intSet.add(new Int(10));
    intSet.add(new Int(20));

    Container<List<String>, HashMap<Int, String>, HashSet<Int>> complexContainer =
        new Container<List<String>, HashMap<Int, String>, HashSet<Int>>(stringList, intStringMap, intSet);

    print("Container first size: " + complexContainer.getFirst().size());
    print("Container second size: " + complexContainer.getSecond().size());
    print("Container third size: " + complexContainer.getThird().size());

    // Test 2: Nested generic wrappers
    print("\nTest 2: Wrapper<Wrapper<Wrapper<String>>>");

    Wrapper<String> innerWrapper = new Wrapper<String>(new String("deeply_wrapped"));
    Wrapper<Wrapper<String>> middleWrapper = new Wrapper<Wrapper<String>>(innerWrapper);
    Wrapper<Wrapper<Wrapper<String>>> outerWrapper = new Wrapper<Wrapper<Wrapper<String>>>(middleWrapper);

    String unwrapped = outerWrapper.getData().getData().getData();
    print("Unwrapped value: " + unwrapped);

    // Test 3: Collections of complex containers
    print("\nTest 3: List<Container<HashSet<String>, List<Int>, HashMap<String, Int>>>");

    List<Container<HashSet<String>, List<Int>, HashMap<String, Int>>> containerList =
        new List<Container<HashSet<String>, List<Int>, HashMap<String, Int>>>();

    HashSet<String> set1 = new HashSet<String>();
    set1.add(new String("item1"));
    set1.add(new String("item2"));

    List<Int> list1 = new List<Int>();
    list1.add(new Int(100));
    list1.add(new Int(200));

    HashMap<String, Int> map1 = new HashMap<String, Int>();
    map1.put(new String("key1"), new Int(300));
    map1.put(new String("key2"), new Int(400));

    Container<HashSet<String>, List<Int>, HashMap<String, Int>> container1 =
        new Container<HashSet<String>, List<Int>, HashMap<String, Int>>(set1, list1, map1);

    containerList.add(container1);

    print("Container list size: " + containerList.size());
    print("First container's first component size: " + containerList.get(0).getFirst().size());

    // Test 4: HashMap with complex key and value types
    print("\nTest 4: HashMap<Container<String, Int, String>, List<HashSet<String>>>");

    HashMap<Container<String, Int, String>, List<HashSet<String>>> complexKeyMap =
        new HashMap<Container<String, Int, String>, List<HashSet<String>>>();

    Container<String, Int, String> keyContainer = new Container<String, Int, String>(
        new String("key_first"),
        new Int(42),
        new String("key_third")
    );

    List<HashSet<String>> valueList = new List<HashSet<String>>();
    HashSet<String> valueSet = new HashSet<String>();
    valueSet.add(new String("value_item"));
    valueList.add(valueSet);

    complexKeyMap.put(keyContainer, valueList);

    print("Complex key map size: " + complexKeyMap.size());

    // Test 5: Stress test with multiple instances of same nested type
    print("\nTest 5: Multiple instances of HashMap<String, List<HashMap<Int, HashSet<String>>>>");

    HashMap<String, List<HashMap<Int, HashSet<String>>>> instance1 =
        new HashMap<String, List<HashMap<Int, HashSet<String>>>>();
    HashMap<String, List<HashMap<Int, HashSet<String>>>> instance2 =
        new HashMap<String, List<HashMap<Int, HashSet<String>>>>();
    HashMap<String, List<HashMap<Int, HashSet<String>>>> instance3 =
        new HashMap<String, List<HashMap<Int, HashSet<String>>>>();

    // Populate instance1
    List<HashMap<Int, HashSet<String>>> mapList = new List<HashMap<Int, HashSet<String>>>();
    HashMap<Int, HashSet<String>> nestedMap = new HashMap<Int, HashSet<String>>();
    HashSet<String> finalSet = new HashSet<String>();
    finalSet.add(new String("final_value"));
    nestedMap.put(new Int(999), finalSet);
    mapList.add(nestedMap);
    instance1.put(new String("test_key"), mapList);

    print("Instance 1 size: " + instance1.size());
    print("Instance 2 size: " + instance2.size());
    print("Instance 3 size: " + instance3.size());

    // Test 6: Maximum complexity test
    print("\nTest 6: Maximum complexity - List<HashMap<Container<String, Int, HashSet<String>>, Wrapper<List<HashMap<String, Int>>>>>");

    List<HashMap<Container<String, Int, HashSet<String>>, Wrapper<List<HashMap<String, Int>>>>> maxComplexity =
        new List<HashMap<Container<String, Int, HashSet<String>>, Wrapper<List<HashMap<String, Int>>>>>();

    HashMap<Container<String, Int, HashSet<String>>, Wrapper<List<HashMap<String, Int>>>> maxMap =
        new HashMap<Container<String, Int, HashSet<String>>, Wrapper<List<HashMap<String, Int>>>>();

    HashSet<String> maxSet = new HashSet<String>();
    maxSet.add(new String("max_item"));

    Container<String, Int, HashSet<String>> maxKey = new Container<String, Int, HashSet<String>>(
        new String("max_key"),
        new Int(1000),
        maxSet
    );

    List<HashMap<String, Int>> maxList = new List<HashMap<String, Int>>();
    HashMap<String, Int> maxInnerMap = new HashMap<String, Int>();
    maxInnerMap.put(new String("max_inner"), new Int(2000));
    maxList.add(maxInnerMap);

    Wrapper<List<HashMap<String, Int>>> maxWrapper = new Wrapper<List<HashMap<String, Int>>>(maxList);

    maxMap.put(maxKey, maxWrapper);
    maxComplexity.add(maxMap);

    print("Maximum complexity size: " + maxComplexity.size());

    print("\n=== All generic complexity stress tests completed successfully! ===");
}

main();