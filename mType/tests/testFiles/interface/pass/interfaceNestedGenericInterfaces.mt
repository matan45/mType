// Test nested generic interfaces
// @Script

import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

interface Outer<T> {
    function getValue(): T;
}

interface Inner<U> {
    function process(U value): U;
}

class Container<T> implements Outer<T> {
    private T value;

    public constructor(T value) {
        this.value = value;
    }

    public function getValue(): T {
        return this.value;
    }
}

class Processor<U> implements Inner<U> {
    public function process(U value): U {
        // Just return the value as-is
        return value;
    }
}

class Combined<T> implements Outer<T>, Inner<T> {
    private T data;

    public constructor(T data) {
        this.data = data;
    }

    public function getValue(): T {
        return this.data;
    }

    public function process(T value): T {
        this.data = value;
        return this.data;
    }
}

Container<String> container = new Container<String>(new String("Hello"));
print(container.getValue().toString());

Processor<Int> processor = new Processor<Int>();
print(processor.process(new Int(42)).toString());

Combined<String> combined = new Combined<String>(new String("World"));
print(combined.getValue().toString());
print(combined.process(new String("Modified")).toString());
