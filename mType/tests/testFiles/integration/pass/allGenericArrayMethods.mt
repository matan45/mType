// All Three Test 2: Generic methods on array-backed classes
@Script

class DataStore<T> {
    T[] data;
    Int count;

    constructor(size: Int) {
        this.data = T[size];
        this.count = 0;
    }

    function push(item: T): void {
        this.data[this.count] = item;
        this.count = this.count + 1;
    }

    function map(func: T -> T): T[] {
        T[] result = T[this.count];
        Int i = 0;
        while (i < this.count) {
            result[i] = func(this.data[i]);
            i = i + 1;
        }
        return result;
    }

    function forEach(action: T -> void): void {
        Int i = 0;
        while (i < this.count) {
            action(this.data[i]);
            i = i + 1;
        }
    }

    function reduce(initial: T, combiner: (T, T) -> T): T {
        T result = initial;
        Int i = 0;
        while (i < this.count) {
            result = combiner(result, this.data[i]);
            i = i + 1;
        }
        return result;
    }

    function getAt(index: Int): T {
        return this.data[index];
    }

    function getCount(): Int {
        return this.count;
    }
}

class Wrapper<T> {
    T value;

    constructor(val: T) {
        this.value = val;
    }

    function getValue(): T {
        return this.value;
    }

    function setValue(val: T): void {
        this.value = val;
    }
}

class WrapperStore<T> {
    Wrapper<T>[] wrappers;
    Int size;

    constructor(capacity: Int) {
        this.wrappers = Wrapper<T>[capacity];
        this.size = 0;
    }

    function add(value: T): void {
        this.wrappers[this.size] = Wrapper<T>(value);
        this.size = this.size + 1;
    }

    function unwrapAll(): T[] {
        T[] result = T[this.size];
        Int i = 0;
        while (i < this.size) {
            result[i] = this.wrappers[i].getValue();
            i = i + 1;
        }
        return result;
    }

    function getWrapper(index: Int): Wrapper<T> {
        return this.wrappers[index];
    }

    function modifyAll(modifier: T -> T): void {
        Int i = 0;
        while (i < this.size) {
            T current = this.wrappers[i].getValue();
            this.wrappers[i].setValue(modifier(current));
            i = i + 1;
        }
    }
}

print("DataStore<Int> with lambdas:");
DataStore<Int> intStore = DataStore<Int>(5);
intStore.push(1);
intStore.push(2);
intStore.push(3);
intStore.push(4);
intStore.push(5);

print("forEach:");
intStore.forEach((x: Int) -> void {
    print(x);
});

print("map (x * 2):");
Int[] doubled = intStore.map((x: Int) -> Int {
    return x * 2;
});
Int i = 0;
while (i < 5) {
    print(doubled[i]);
    i = i + 1;
}

print("reduce (sum):");
Int sum = intStore.reduce(0, (acc: Int, x: Int) -> Int {
    return acc + x;
});
print(sum);

print("WrapperStore<String>:");
WrapperStore<String> strWrapperStore = WrapperStore<String>(3);
strWrapperStore.add("hello");
strWrapperStore.add("world");
strWrapperStore.add("test");

print("Unwrapped:");
String[] unwrapped = strWrapperStore.unwrapAll();
i = 0;
while (i < 3) {
    print(unwrapped[i]);
    i = i + 1;
}

print("Direct wrapper access:");
print(strWrapperStore.getWrapper(1).getValue());

print("WrapperStore<Int>:");
WrapperStore<Int> intWrapperStore = WrapperStore<Int>(4);
intWrapperStore.add(10);
intWrapperStore.add(20);
intWrapperStore.add(30);
intWrapperStore.add(40);

intWrapperStore.modifyAll((x: Int) -> Int {
    return x + 5;
});

print("After modification:");
Int[] modified = intWrapperStore.unwrapAll();
i = 0;
while (i < 4) {
    print(modified[i]);
    i = i + 1;
}
