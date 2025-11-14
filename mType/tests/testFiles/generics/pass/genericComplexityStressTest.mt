import * from "../../lib/collections/ArrayList.mt";
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
    print("\nTest 1: Container<ArrayList<String>, HashMap<Int, String>, HashSet<Int>>");

    ArrayList<String> stringArrayList = new ArrayList<String>();
    stringArrayList.add(new String("test1"));
    stringArrayList.add(new String("test2"));

    HashMap<Int, String> intStringMap = new HashMap<Int, String>();
    intStringMap.put(new Int(1), new String("one"));
    intStringMap.put(new Int(2), new String("two"));

    HashSet<Int> intSet = new HashSet<Int>();
    intSet.add(new Int(10));
    intSet.add(new Int(20));

    Container<ArrayList<String>, HashMap<Int, String>, HashSet<Int>> complexContainer =
        new Container<ArrayList<String>, HashMap<Int,String>, HashSet<Int>>(stringArrayList, intStringMap, intSet);

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
    print("\nTest 3: ArrayList<Container<HashSet<String>, ArrayList<Int>, HashMap<String, Int>>>");

    ArrayList<Container<HashSet<String>, ArrayList<Int>, HashMap<String, Int>>> containerArrayList =
        new ArrayList<Container<HashSet<String>, ArrayList<Int>, HashMap<String, Int>>>();

    HashSet<String> set1 = new HashSet<String>();
    set1.add(new String("item1"));
    set1.add(new String("item2"));

    ArrayList<Int> ArrayList1 = new ArrayList<Int>();
    ArrayList1.add(new Int(100));
    ArrayList1.add(new Int(200));

    HashMap<String, Int> map1 = new HashMap<String, Int>();
    map1.put(new String("key1"), new Int(300));
    map1.put(new String("key2"), new Int(400));

    Container<HashSet<String>, ArrayList<Int>, HashMap<String, Int>> container1 =
        new Container<HashSet<String>, ArrayList<Int>, HashMap<String, Int>>(set1, ArrayList1, map1);

    containerArrayList.add(container1);

    print("Container ArrayList size: " + containerArrayList.size());
    print("First container's first component size: " + containerArrayList.get(0).getFirst().size());

    // Test 4: HashMap with complex key and value types
    print("\nTest 4: HashMap<Container<String, Int, String>, ArrayList<HashSet<String>>>");

    HashMap<Container<String, Int, String>, ArrayList<HashSet<String>>> complexKeyMap =
        new HashMap<Container<String, Int, String>, ArrayList<HashSet<String>>>();

    Container<String, Int, String> keyContainer = new Container<String, Int, String>(
        new String("key_first"),
        new Int(42),
        new String("key_third")
    );

    ArrayList<HashSet<String>> valueArrayList = new ArrayList<HashSet<String>>();
    HashSet<String> valueSet = new HashSet<String>();
    valueSet.add(new String("value_item"));
    valueArrayList.add(valueSet);

    complexKeyMap.put(keyContainer, valueArrayList);

    print("Complex key map size: " + complexKeyMap.size());

    // Test 5: Stress test with multiple instances of same nested type
    print("\nTest 5: Multiple instances of HashMap<String, ArrayList<HashMap<Int, HashSet<String>>>>");

    HashMap<String, ArrayList<HashMap<Int, HashSet<String>>>> instance1 =
        new HashMap<String, ArrayList<HashMap<Int, HashSet<String>>>>();
    HashMap<String, ArrayList<HashMap<Int, HashSet<String>>>> instance2 =
        new HashMap<String, ArrayList<HashMap<Int, HashSet<String>>>>();
    HashMap<String, ArrayList<HashMap<Int, HashSet<String>>>> instance3 =
        new HashMap<String, ArrayList<HashMap<Int, HashSet<String>>>>();

    // Populate instance1
    ArrayList<HashMap<Int, HashSet<String>>> mapArrayList = new ArrayList<HashMap<Int, HashSet<String>>>();
    HashMap<Int, HashSet<String>> nestedMap = new HashMap<Int, HashSet<String>>();
    HashSet<String> finalSet = new HashSet<String>();
    finalSet.add(new String("final_value"));
    nestedMap.put(new Int(999), finalSet);
    mapArrayList.add(nestedMap);
    instance1.put(new String("test_key"), mapArrayList);

    print("Instance 1 size: " + instance1.size());
    print("Instance 2 size: " + instance2.size());
    print("Instance 3 size: " + instance3.size());

    // Test 6: Maximum complexity test
    print("\nTest 6: Maximum complexity - ArrayList<HashMap<Container<String, Int, HashSet<String>>, Wrapper<ArrayList<HashMap<String, Int>>>>>");

    ArrayList<HashMap<Container<String, Int, HashSet<String>>, Wrapper<ArrayList<HashMap<String, Int>>>>> maxComplexity =
        new ArrayList<HashMap<Container<String, Int, HashSet<String>>, Wrapper<ArrayList<HashMap<String, Int>>>>>();

    HashMap<Container<String, Int, HashSet<String>>, Wrapper<ArrayList<HashMap<String, Int>>>> maxMap =
        new HashMap<Container<String, Int, HashSet<String>>, Wrapper<ArrayList<HashMap<String, Int>>>>();

    HashSet<String> maxSet = new HashSet<String>();
    maxSet.add(new String("max_item"));

    Container<String, Int, HashSet<String>> maxKey = new Container<String, Int, HashSet<String>>(
        new String("max_key"),
        new Int(1000),
        maxSet
    );

    ArrayList<HashMap<String, Int>> maxArrayList = new ArrayList<HashMap<String, Int>>();
    HashMap<String, Int> maxInnerMap = new HashMap<String, Int>();
    maxInnerMap.put(new String("max_inner"), new Int(2000));
    maxArrayList.add(maxInnerMap);

    Wrapper<ArrayList<HashMap<String, Int>>> maxWrapper = new Wrapper<ArrayList<HashMap<String, Int>>>(maxArrayList);

    maxMap.put(maxKey, maxWrapper);
    maxComplexity.add(maxMap);

    print("Maximum complexity size: " + maxComplexity.size());

    print("\n=== All generic complexity stress tests completed successfully! ===");
}

main();