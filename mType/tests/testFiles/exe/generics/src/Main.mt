// Test: Generics in standalone exe
// Generic class, generic method, Pair<K,V>

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
        // Generic class with int
        Box<int> intBox = new Box<int>(42);
        print("Int box: " + intBox.getValue());

        // Generic class with string
        Box<string> strBox = new Box<string>("hello");
        print("String box: " + strBox.getValue());

        // Pair with different types
        Pair<string, int> pair = new Pair<string, int>("age", 25);
        print("Pair: " + pair.getFirst() + " = " + pair.getSecond());

        // Nested generics
        Box<Box<int>> nested = new Box<Box<int>>(new Box<int>(99));
        print("Nested: " + nested.getValue().getValue());

        print("Generics test passed");
    }
}
