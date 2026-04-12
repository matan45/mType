// Test: Generics in standalone exe
import * from "primitives/Int.mt";
import * from "primitives/String.mt";

class Box<T> {
    private T value;

    public constructor(T val) {
        this.value = val;
    }

    public function getValue(): T {
        return this.value;
    }

    public function toString(): string {
        return "Box(" + this.value + ")";
    }
}

class Pair<K, V> {
    private K first;
    private V second;

    public constructor(K f, V s) {
        this.first = f;
        this.second = s;
    }

    public function getFirst(): K {
        return this.first;
    }

    public function getSecond(): V {
        return this.second;
    }
}

@EntryPoint
class App {
    public static function main(string[] args): void {
        // Generic class with boxed Int
        Box<Int> intBox = new Box<Int>(new Int(42));
        print("Int box: " + intBox.getValue());

        // Generic class with boxed String
        Box<String> strBox = new Box<String>(new String("hello"));
        print("String box: " + strBox.getValue());

        // Pair with different types
        Pair<String, Int> pair = new Pair<String, Int>(new String("age"), new Int(25));
        print("Pair: " + pair.getFirst() + " = " + pair.getSecond());

        // Nested generics
        Box<Box<Int>> nested = new Box<Box<Int>>(new Box<Int>(new Int(99)));
        print("Nested: " + nested.getValue().getValue());

        print("Generics test passed");
    }
}
