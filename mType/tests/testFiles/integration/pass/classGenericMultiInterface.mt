// Class + Generics Test 2: Multiple generic interface implementation
@Script

interface Readable<T> {
    function read(): T;
}

interface Writable<T> {
    function write(T value): void;
}

interface Comparable<T> {
    function compareTo(T other): Int;
}

class Container<T> implements Readable<T>, Writable<T> {
    T data;
    Bool hasData;

    constructor() {
        this.hasData = false;
    }

    public function read(): T {
        return this.data;
    }

    public function write(T value): void {
        this.data = value;
        this.hasData = true;
    }

    public function isEmpty(): Bool {
        return !this.hasData;
    }
}

class ComparableValue<T> implements Comparable<ComparableValue<T>> {
    T value;

    constructor(T val) {
        this.value = val;
    }

    public function compareTo(ComparableValue<T> other): Int {
        return 0; // Simplified for testing
    }

    public function getValue(): T {
        return this.value;
    }
}

print("Testing Container with Int:");
Container<Int> intContainer = Container<Int>();
print(intContainer.isEmpty());

intContainer.write(42);
print(intContainer.isEmpty());
print(intContainer.read());

print("Testing Container with String:");
Container<String> strContainer = Container<String>();
strContainer.write("hello");
print(strContainer.read());

print("Testing ComparableValue:");
ComparableValue<Int> cv1 = ComparableValue<Int>(10);
ComparableValue<Int> cv2 = ComparableValue<Int>(20);
print(cv1.getValue());
print(cv2.getValue());
print(cv1.compareTo(cv2));

print("Nested generics:");
Container<ComparableValue<String>> nestedContainer = Container<ComparableValue<String>>();
nestedContainer.write(ComparableValue<String>("test"));
print(nestedContainer.read().getValue());
