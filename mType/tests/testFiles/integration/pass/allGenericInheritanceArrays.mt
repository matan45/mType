// All Three Test 3: Inheritance with generic array types
@Script

class BaseContainer<T> {
    field items: T[];
    field capacity: Int;

    constructor(cap: Int) {
        this.capacity = cap;
        this.items = T[cap];
    }

    fun set(index: Int, value: T): Void {
        this.items[index] = value;
    }

    fun get(index: Int): T {
        return this.items[index];
    }

    fun getCapacity(): Int {
        return this.capacity;
    }

    fun describe(): String {
        return "BaseContainer";
    }
}

class AdvancedContainer<T> extends BaseContainer<T> {
    field activeCount: Int;

    constructor(cap: Int) {
        super(cap);
        this.activeCount = 0;
    }

    fun add(value: T): Void {
        if (this.activeCount < this.capacity) {
            this.set(this.activeCount, value);
            this.activeCount = this.activeCount + 1;
        }
    }

    fun getActiveCount(): Int {
        return this.activeCount;
    }

    fun toArray(): T[] {
        let result: T[] = T[this.activeCount];
        let i: Int = 0;
        while (i < this.activeCount) {
            result[i] = this.get(i);
            i = i + 1;
        }
        return result;
    }

    fun describe(): String {
        return "AdvancedContainer";
    }
}

class SmartContainer<T, U> extends AdvancedContainer<T> {
    field metadata: U[];
    field metaCount: Int;

    constructor(cap: Int, metaCap: Int) {
        super(cap);
        this.metadata = U[metaCap];
        this.metaCount = 0;
    }

    fun addWithMeta(value: T, meta: U): Void {
        this.add(value);
        this.metadata[this.metaCount] = meta;
        this.metaCount = this.metaCount + 1;
    }

    fun getMeta(index: Int): U {
        return this.metadata[index];
    }

    fun describe(): String {
        return "SmartContainer";
    }
}

print("Testing BaseContainer<Int>:");
let base: BaseContainer<Int> = BaseContainer<Int>(3);
base.set(0, 10);
base.set(1, 20);
base.set(2, 30);

print(base.describe());
let i: Int = 0;
while (i < 3) {
    print(base.get(i));
    i = i + 1;
}

print("Testing AdvancedContainer<String>:");
let advanced: AdvancedContainer<String> = AdvancedContainer<String>(4);
advanced.add("alpha");
advanced.add("beta");
advanced.add("gamma");

print(advanced.describe());
print(advanced.getActiveCount());

let arr: String[] = advanced.toArray();
i = 0;
while (i < advanced.getActiveCount()) {
    print(arr[i]);
    i = i + 1;
}

print("Testing SmartContainer<Int, String>:");
let smart: SmartContainer<Int, String> = SmartContainer<Int, String>(3, 3);
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
let baseRef: BaseContainer<Int> = AdvancedContainer<Int>(2);
baseRef.set(0, 99);
baseRef.set(1, 88);
print(baseRef.describe());
print(baseRef.get(0));
print(baseRef.get(1));

let advRef: AdvancedContainer<Float> = SmartContainer<Float, Int>(2, 2);
advRef.add(1.5);
advRef.add(2.5);
print(advRef.describe());
print(advRef.getActiveCount());
