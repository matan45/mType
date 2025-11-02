// Test nested generic interfaces
// @Script

interface Outer<T> {
    func getValue(): T;
}

interface Inner<U> {
    func process(value: U): U;
}

class Container<T> implements Outer<T> {
    var value: T;

    func init(value: T) {
        this.value = value;
    }

    func getValue(): T {
        return this.value;
    }
}

class Processor<U> implements Inner<U> {
    func process(value: U): U {
        // Just return the value as-is
        return value;
    }
}

class Combined<T> implements Outer<T>, Inner<T> {
    var data: T;

    func init(data: T) {
        this.data = data;
    }

    func getValue(): T {
        return this.data;
    }

    func process(value: T): T {
        this.data = value;
        return this.data;
    }
}

var container = new Container<String>("Hello");
print(container.getValue());

var processor = new Processor<Int>();
print(processor.process(42));

var combined = new Combined<String>("World");
print(combined.getValue());
print(combined.process("Modified"));
