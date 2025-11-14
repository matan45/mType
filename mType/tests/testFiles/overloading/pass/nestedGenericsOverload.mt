// Test overloading with nested generic types
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

// Generic container classes
class Box<T> {
    T value;

    constructor(T val) {
        this.value = val;
    }

    public function getValue(): T {
        return this.value;
    }
}

class Pair<K, V> {
    K key;
    V value;

    constructor(K k, V v) {
        this.key = k;
        this.value = v;
    }

    public function getKey(): K {
        return this.key;
    }

    public function getValue(): V {
        return this.value;
    }
}

// Overloaded functions with nested generics
function process(Box<Int> box): string {
    return "Box<Int>: " + box.getValue();
}

function process(Box<String> box): string {
    return "Box<String>: " + box.getValue();
}

function process(Box<Box<Int>> nestedBox): string {
    return "Box<Box<Int>>: " + nestedBox.getValue().getValue();
}

function <T> process(Box<T> box): string {
    return "Box<T>: " + box.getValue();
}

// Overloaded functions with multi-parameter generics
function processPair(Pair<Int, String> pair): string {
    return "Pair<Int,String>: " + pair.getKey() + " -> " + pair.getValue();
}

function processPair(Pair<String, Int> pair): string {
    return "Pair<String,Int>: " + pair.getKey() + " -> " + pair.getValue();
}

function processPair(Pair<Int, Int> pair): string {
    return "Pair<Int,Int>: " + pair.getKey() + " -> " + pair.getValue();
}

function <K, V> processPair(Pair<K, V> pair): string {
    return "Pair<K,V>: " + pair.getKey() + " -> " + pair.getValue();
}

// Overloaded functions with nested multi-parameter generics
function processNested(Box<Pair<Int, String>> box): string {
    Pair<Int, String> p = box.getValue();
    return "Box<Pair<Int,String>>: " + p.getKey() + " -> " + p.getValue();
}

function <T> processNested(Box<Pair<T, String>> box): string {
    return "Box<Pair<T,String>>: generic key";
}

function <K, V> processNested(Box<Pair<K, V>> box): string {
    return "Box<Pair<K,V>>: fully generic";
}

function main(): void {
    print("=== Nested Generic Overload Tests ===");

    // Test single-parameter generic overloads
    Box<Int> intBox = new Box<Int>(42);
    Box<String> strBox = new Box<String>("Hello");
    Box<Box<Int>> nestedIntBox = new Box<Box<Int>>(new Box<Int>(99));
    Box<Bool> boolBox = new Box<Bool>(true);

    print(process(intBox));           // Should call Box<Int> overload
    print(process(strBox));           // Should call Box<String> overload
    print(process(nestedIntBox));     // Should call Box<Box<Int>> overload
    print(process<Box<Bool>>(boolBox)); // Should call Box<T> overload with explicit type

    print("");
    print("=== Multi-Parameter Generic Overload Tests ===");

    // Test multi-parameter generic overloads
    Pair<Int, String> intStrPair = new Pair<Int, String>(1, "one");
    Pair<String, Int> strIntPair = new Pair<String, Int>("two", 2);
    Pair<Int, Int> intIntPair = new Pair<Int, Int>(3, 4);
    Pair<Bool, String> boolStrPair = new Pair<Bool, String>(true, "yes");

    print(processPair(intStrPair));   // Should call Pair<Int,String> overload
    print(processPair(strIntPair));   // Should call Pair<String,Int> overload
    print(processPair(intIntPair));   // Should call Pair<Int,Int> overload
    print(processPair<Bool, String>(boolStrPair)); // Should call Pair<K,V> overload

    print("");
    print("=== Nested Multi-Parameter Generic Tests ===");

    // Test nested multi-parameter generics
    Box<Pair<Int, String>> boxPair1 = new Box<Pair<Int, String>>(new Pair<Int, String>(5, "five"));
    Box<Pair<Bool, String>> boxPair2 = new Box<Pair<Bool, String>>(new Pair<Bool, String>(false, "no"));
    Box<Pair<String, Int>> boxPair3 = new Box<Pair<String, Int>>(new Pair<String, Int>("six", 6));

    print(processNested(boxPair1));   // Should call Box<Pair<Int,String>> overload
    print(processNested<Bool>(boxPair2)); // Should call Box<Pair<T,String>> overload
    print(processNested<String, Int>(boxPair3)); // Should call Box<Pair<K,V>> overload

    print("");
    print("All nested generic overload tests passed!");
}

main();
