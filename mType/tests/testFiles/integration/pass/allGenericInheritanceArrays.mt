// All Three Test 3: Inheritance with generic array types
@Script

class BaseContainer<T> {
    T[] items;
    Int capacity;

    constructor(cap: Int) {
        this.capacity = cap;
        this.items = T[cap];
    }

    function set(index: Int, value: T): void {
        this.items[index] = value;
    }

    function get(index: Int): T {
        return this.items[index];
    }

    function getCapacity(): Int {
        return this.capacity;
    }

    function describe(): String {
        return "BaseContainer";
    }
}

class AdvancedContainer<T> extends BaseContainer<T> {
    Int activeCount;

    constructor(cap: Int) {
        super(cap);
        this.activeCount = 0;
    }

    function add(value: T): void {
        if (this.activeCount < this.capacity) {
            this.set(this.activeCount, value);
            this.activeCount = this.activeCount + 1;
        }
    }

    function getActiveCount(): Int {
        return this.activeCount;
    }

    function toArray(): T[] {
        T[] result = T[this.activeCount];
        Int i = 0;
        while (i < this.activeCount) {
            result[i] = this.get(i);
            i = i + 1;
        }
        return result;
    }

    function describe(): String {
        return "AdvancedContainer";
    }
}

class SmartContainer<T, U> extends AdvancedContainer<T> {
    U[] metadata;
    Int metaCount;

    constructor(cap: Int, metaCap: Int) {
        super(cap);
        this.metadata = U[metaCap];
        this.metaCount = 0;
    }

    function addWithMeta(value: T, meta: U): void {
        this.add(value);
        this.metadata[this.metaCount] = meta;
        this.metaCount = this.metaCount + 1;
    }

    function getMeta(index: Int): U {
        return this.metadata[index];
    }

    function describe(): String {
        return "SmartContainer";
    }
}

print("Testing BaseContainer<Int>:");
BaseContainer<Int> base = BaseContainer<Int>(3);
base.set(0, 10);
base.set(1, 20);
base.set(2, 30);

print(base.describe());
Int i = 0;
while (i < 3) {
    print(base.get(i));
    i = i + 1;
}

print("Testing AdvancedContainer<String>:");
AdvancedContainer<String> advanced = AdvancedContainer<String>(4);
advanced.add("alpha");
advanced.add("beta");
advanced.add("gamma");

print(advanced.describe());
print(advanced.getActiveCount());

String[] arr = advanced.toArray();
i = 0;
while (i < advanced.getActiveCount()) {
    print(arr[i]);
    i = i + 1;
}

print("Testing SmartContainer<Int, String>:");
SmartContainer<Int, String> smart = SmartContainer<Int, String>(3, 3);
smart.addWithMeta(100, "first");
smart.addWithMeta(200, "second");
smart.addWithMeta(300, "third");

print(smart.describe());
print(smart.getActiveCount());

i = 0;
while (i < smart.getActiveCount()) {
    print(smart.get(i));
    print(smart.getMeta(i));
    i = i + 1;
}

print("Polymorphism test:");
BaseContainer<Int> baseRef = AdvancedContainer<Int>(2);
baseRef.set(0, 99);
baseRef.set(1, 88);
print(baseRef.describe());
print(baseRef.get(0));
print(baseRef.get(1));

AdvancedContainer<Float> advRef = SmartContainer<Float, Int>(2, 2);
advRef.add(1.5);
advRef.add(2.5);
print(advRef.describe());
print(advRef.getActiveCount());
