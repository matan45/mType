// All Three Test 2: Generic methods on array-backed classes
@Script

class DataStore<T> {
    field data: T[];
    field count: Int;

    constructor(size: Int) {
        this.data = T[size];
        this.count = 0;
    }

    fun push(item: T): Void {
        this.data[this.count] = item;
        this.count = this.count + 1;
    }

    fun map(func: T -> T): T[] {
        let result: T[] = T[this.count];
        let i: Int = 0;
        while (i < this.count) {
            result[i] = func(this.data[i]);
            i = i + 1;
        }
        return result;
    }

    fun forEach(action: T -> Void): Void {
        let i: Int = 0;
        while (i < this.count) {
            action(this.data[i]);
            i = i + 1;
        }
    }

    fun reduce(initial: T, combiner: (T, T) -> T): T {
        let result: T = initial;
        let i: Int = 0;
        while (i < this.count) {
            result = combiner(result, this.data[i]);
            i = i + 1;
        }
        return result;
    }

    fun getAt(index: Int): T {
        return this.data[index];
    }

    fun getCount(): Int {
        return this.count;
    }
}

class Wrapper<T> {
    field value: T;

    constructor(val: T) {
        this.value = val;
    }

    fun getValue(): T {
        return this.value;
    }

    fun setValue(val: T): Void {
        this.value = val;
    }
}

class WrapperStore<T> {
    field wrappers: Wrapper<T>[];
    field size: Int;

    constructor(capacity: Int) {
        this.wrappers = Wrapper<T>[capacity];
        this.size = 0;
    }

    fun add(value: T): Void {
        this.wrappers[this.size] = Wrapper<T>(value);
        this.size = this.size + 1;
    }

    fun unwrapAll(): T[] {
        let result: T[] = T[this.size];
        let i: Int = 0;
        while (i < this.size) {
            result[i] = this.wrappers[i].getValue();
            i = i + 1;
        }
        return result;
    }

    fun getWrapper(index: Int): Wrapper<T> {
        return this.wrappers[index];
    }

    fun modifyAll(modifier: T -> T): Void {
        let i: Int = 0;
        while (i < this.size) {
            let current: T = this.wrappers[i].getValue();
            this.wrappers[i].setValue(modifier(current));
            i = i + 1;
        }
    }
}

print("DataStore<Int> with lambdas:");
let intStore: DataStore<Int> = DataStore<Int>(5);
intStore.push(1);
intStore.push(2);
intStore.push(3);
intStore.push(4);
intStore.push(5);

print("forEach:");
intStore.forEach((x: Int) -> Void {
    print(x);
});

print("map (x * 2):");
let doubled: Int[] = intStore.map((x: Int) -> Int {
    return x * 2;
});
let i: Int = 0;
while (i < 5) {
    print(doubled[i]);
    i = i + 1;
}

print("reduce (sum):");
let sum: Int = intStore.reduce(0, (acc: Int, x: Int) -> Int {
    return acc + x;
});
print(sum);

print("WrapperStore<String>:");
let strWrapperStore: WrapperStore<String> = WrapperStore<String>(3);
strWrapperStore.add("hello");
strWrapperStore.add("world");
strWrapperStore.add("test");

print("Unwrapped:");
let unwrapped: String[] = strWrapperStore.unwrapAll();
i = 0;
while (i < 3) {
    print(unwrapped[i]);
    i = i + 1;
}

print("Direct wrapper access:");
print(strWrapperStore.getWrapper(1).getValue());

print("WrapperStore<Int>:");
let intWrapperStore: WrapperStore<Int> = WrapperStore<Int>(4);
intWrapperStore.add(10);
intWrapperStore.add(20);
intWrapperStore.add(30);
intWrapperStore.add(40);

intWrapperStore.modifyAll((x: Int) -> Int {
    return x + 5;
});

print("After modification:");
let modified: Int[] = intWrapperStore.unwrapAll();
i = 0;
while (i < 4) {
    print(modified[i]);
    i = i + 1;
}
