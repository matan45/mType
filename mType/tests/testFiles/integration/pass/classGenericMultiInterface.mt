// Class + Generics Test 2: Multiple generic interface implementation
@Script

interface Readable<T> {
    fun read(): T;
}

interface Writable<T> {
    fun write(value: T): Void;
}

interface Comparable<T> {
    fun compareTo(other: T): Int;
}

class Container<T> implements Readable<T>, Writable<T> {
    field data: T;
    field hasData: Bool;

    constructor() {
        this.hasData = false;
    }

    fun read(): T {
        return this.data;
    }

    fun write(value: T): Void {
        this.data = value;
        this.hasData = true;
    }

    fun isEmpty(): Bool {
        return !this.hasData;
    }
}

class ComparableValue<T> implements Comparable<ComparableValue<T>> {
    field value: T;

    constructor(val: T) {
        this.value = val;
    }

    fun compareTo(other: ComparableValue<T>): Int {
        return 0; // Simplified for testing
    }

    fun getValue(): T {
        return this.value;
    }
}

print("Testing Container with Int:");
let intContainer: Container<Int> = Container<Int>();
print(intContainer.isEmpty());

intContainer.write(42);
print(intContainer.isEmpty());
print(intContainer.read());

print("Testing Container with String:");
let strContainer: Container<String> = Container<String>();
strContainer.write("hello");
print(strContainer.read());

print("Testing ComparableValue:");
let cv1: ComparableValue<Int> = ComparableValue<Int>(10);
let cv2: ComparableValue<Int> = ComparableValue<Int>(20);
print(cv1.getValue());
print(cv2.getValue());
print(cv1.compareTo(cv2));

print("Nested generics:");
let nestedContainer: Container<ComparableValue<String>> = Container<ComparableValue<String>>();
nestedContainer.write(ComparableValue<String>("test"));
print(nestedContainer.read().getValue());
