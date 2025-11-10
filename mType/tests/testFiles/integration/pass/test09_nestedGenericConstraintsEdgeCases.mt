// Integration Test 09: Nested Generic Constraints Edge Cases
// Tests: Complex generic hierarchies with constraints

import * from "../../lib/collections/List.mt";
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

// Base interface for comparability
interface Comparable<T> {
    function compareTo(T other): int;
}

// Serializable interface
interface Serializable {
    function serialize(): string;
}

// Combined interface
interface ComparableSerializable<T> extends Comparable<T> {
    function serialize(): string;
}

// Data class implementing multiple interfaces
class DataItem implements ComparableSerializable<DataItem> {
    private int value;
    private string name;

    constructor(int v, string n) {
        this.value = v;
        this.name = n;
    }

    public function compareTo(DataItem other): int {
        if (this.value < other.getValue()) {
            return -1;
        } else if (this.value > other.getValue()) {
            return 1;
        }
        return 0;
    }

    public function serialize(): string {
        return "DataItem{value=" + this.value + ",name=" + this.name + "}";
    }

    public function getValue(): int {
        return this.value;
    }

    public function getName(): string {
        return this.name;
    }
}

// Generic container with constraint
class Container<T extends Comparable<T>> {
    protected T item;

    constructor(T i) {
        this.item = i;
    }

    public function getItem(): T {
        return this.item;
    }

    public function isGreaterThan(T other): bool {
        return this.item.compareTo(other) > 0;
    }
}

// Nested generic with multiple constraints
class SerializableContainer<T extends ComparableSerializable<T>> extends Container<T> {
    constructor(T i) : super(i) {}

    public function serializeItem(): string {
        return this.item.serialize();
    }
}

// Generic pair with different constraints
class ComparablePair<T extends Comparable<T>, U extends Comparable<U>> {
    private T first;
    private U second;

    constructor(T f, U s) {
        this.first = f;
        this.second = s;
    }

    public function getFirst(): T {
        return this.first;
    }

    public function getSecond(): U {
        return this.second;
    }

    public function compareFirst(T other): int {
        return this.first.compareTo(other);
    }

    public function compareSecond(U other): int {
        return this.second.compareTo(other);
    }
}

// Generic wrapper for containers
class ContainerWrapper<T extends Comparable<T>> {
    private Container<T> container;

    constructor(Container<T> c) {
        this.container = c;
    }

    public function getContainer(): Container<T> {
        return this.container;
    }

    public function compareWith(T value): bool {
        return this.container.isGreaterThan(value);
    }
}

// Test with List of generic containers
class ContainerCollection<T extends Comparable<T>> {
    private List<Container<T>> containers;

    constructor() {
        this.containers = new List<Container<T>>();
    }

    public function add(Container<T> container): void {
        this.containers.add(container);
    }

    public function findMax(): T {
        if (this.containers.size() == 0) {
            return null;
        }

        T max = this.containers.get(0).getItem();

        for (int i = 1; i < this.containers.size(); i = i + 1) {
            Container<T> current = this.containers.get(i);
            T item = current.getItem();

            if (item.compareTo(max) > 0) {
                max = item;
            }
        }

        return max;
    }

    public function getSize(): int {
        return this.containers.size();
    }
}

// Main test execution
print("=== Test 09: Nested Generic Constraints Edge Cases ===");

// Test 1: Basic constraint satisfaction
print("--- Basic constraint satisfaction ---");
DataItem item1 = new DataItem(100, "First");
DataItem item2 = new DataItem(50, "Second");

Container<DataItem> container1 = new Container<DataItem>(item1);
print("Container item: " + container1.getItem().getName());

bool isGreater = container1.isGreaterThan(item2);
print("item1 > item2: " + isGreater);

// Test 2: Nested generics with SerializableContainer
print("--- Nested generics ---");
SerializableContainer<DataItem> serContainer = new SerializableContainer<DataItem>(item1);
print("Serialized: " + serContainer.serializeItem());
print("Is greater than item2: " + serContainer.isGreaterThan(item2));

// Test 3: Generic pair with different constraints
print("--- Generic pair ---");
DataItem pairItem1 = new DataItem(75, "Alpha");
DataItem pairItem2 = new DataItem(125, "Beta");

ComparablePair<DataItem, DataItem> pair = new ComparablePair<DataItem, DataItem>(pairItem1, pairItem2);
print("First: " + pair.getFirst().getName());
print("Second: " + pair.getSecond().getName());

int compResult = pair.compareFirst(pairItem2);
if (compResult < 0) {
    print("First < comparison item");
} else if (compResult > 0) {
    print("First > comparison item");
} else {
    print("First == comparison item");
}

// Test 4: Wrapped containers
print("--- Container wrapper ---");
Container<DataItem> innerContainer = new Container<DataItem>(pairItem2);
ContainerWrapper<DataItem> wrapper = new ContainerWrapper<DataItem>(innerContainer);

bool wrapperComp = wrapper.compareWith(pairItem1);
print("Wrapped container comparison: " + wrapperComp);

// Test 5: Collection of containers
print("--- Container collection ---");
ContainerCollection<DataItem> collection = new ContainerCollection<DataItem>();

collection.add(new Container<DataItem>(new DataItem(30, "Low")));
collection.add(new Container<DataItem>(new DataItem(90, "High")));
collection.add(new Container<DataItem>(new DataItem(60, "Medium")));

print("Collection size: " + collection.getSize());

DataItem maxItem = collection.findMax();
if (maxItem != null) {
    print("Max item: " + maxItem.getName() + " = " + maxItem.getValue());
}

// Test 6: Chained generic operations
print("--- Chained generic operations ---");
DataItem chain1 = new DataItem(15, "Chain1");
DataItem chain2 = new DataItem(45, "Chain2");
DataItem chain3 = new DataItem(30, "Chain3");

Container<DataItem> c1 = new Container<DataItem>(chain1);
Container<DataItem> c2 = new Container<DataItem>(chain2);
Container<DataItem> c3 = new Container<DataItem>(chain3);

print("c2 > c1: " + c2.isGreaterThan(chain1));
print("c2 > c3: " + c2.isGreaterThan(chain3));
print("c3 > c1: " + c3.isGreaterThan(chain1));

// Test 7: Null handling with generics
print("--- Null handling ---");
Container<DataItem> nullContainer = new Container<DataItem>(chain1);
DataItem retrieved = nullContainer.getItem();

if (retrieved != null) {
    print("Retrieved item: " + retrieved.getName());
} else {
    print("Retrieved null");
}

print("=== Test 09 Complete ===");
