// Test: Nested generic type arguments in inheritance
// This tests the critical fix for ObjectInstanceHelper and SuperCallHandler
// to properly parse nested generics like "Container<HashMap<String, List<Int>>>"

// Define base classes
class List<T> {
    private T[] items;
    private int size;

    constructor() {
        this.size = 0;
    }

    public function add(T item): void {
        this.size = this.size + 1;
    }

    public function getSize(): int {
        return this.size;
    }
}

class HashMap<K, V> {
    private int count;

    constructor() {
        this.count = 0;
    }

    public function put(K key, V value): void {
        this.count = this.count + 1;
    }

    public function getCount(): int {
        return this.count;
    }
}

class Container<T> {
    protected T data;

    constructor(T initialData) {
        this.data = initialData;
    }

    public function getData(): T {
        return this.data;
    }
}

// Test 1: Two-level nested generics in inheritance
class MapContainer extends Container<HashMap<String, Int>> {
    constructor(): super(new HashMap<String, Int>()) {
    }

    public function testMapOperations(): int {
        HashMap<String, Int> map = super.getData();
        map.put("key1", 10);
        map.put("key2", 20);
        return map.getCount();
    }
}

// Test 2: Three-level nested generics in inheritance
class ComplexContainer extends Container<HashMap<String, List<Int>>> {
    constructor(): super(new HashMap<String, List<Int>>()) {
    }

    public function testComplexOperations(): int {
        HashMap<String, List<Int>> map = super.getData();
        List<Int> list1 = new List<Int>();
        list1.add(1);
        list1.add(2);
        list1.add(3);

        List<Int> list2 = new List<Int>();
        list2.add(10);
        list2.add(20);

        map.put("list1", list1);
        map.put("list2", list2);

        return map.getCount();
    }
}

// Test 3: Multiple generic parameters with nesting
class Pair<A, B> {
    protected A first;
    protected B second;

    constructor(A a, B b) {
        this.first = a;
        this.second = b;
    }

    public function getFirst(): A {
        return this.first;
    }

    public function getSecond(): B {
        return this.second;
    }
}

class NestedPairContainer extends Container<Pair<List<String>, HashMap<Int, String>>> {
    constructor(): super(new Pair<List<String>, HashMap<Int, String>>(
        new List<String>(),
        new HashMap<Int, String>()
    )) {
    }

    public function testNestedPair(): int {
        Pair<List<String>, HashMap<Int, String>> pair = super.getData();
        List<String> list = pair.getFirst();
        list.add("hello");
        list.add("world");

        HashMap<Int, String> map = pair.getSecond();
        map.put(1, "one");
        map.put(2, "two");
        map.put(3, "three");

        return list.getSize() + map.getCount();
    }
}

function main(): void {
    print("Test 1: Two-level nested generics (HashMap<String, Int>)");
    MapContainer mc = new MapContainer();
    print("Map operations count: " + mc.testMapOperations());

    print("\nTest 2: Three-level nested generics (HashMap<String, List<Int>>)");
    ComplexContainer cc = new ComplexContainer();
    print("Complex operations count: " + cc.testComplexOperations());

    print("\nTest 3: Multiple parameters with deep nesting");
    NestedPairContainer npc = new NestedPairContainer();
    print("Nested pair total: " + npc.testNestedPair());

    print("\nAll nested generic inheritance tests passed!");
}

main();
