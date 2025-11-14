// Test deeply nested generic interfaces
// @Script

import * from "../../lib/collections/ArrayList.mt";
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

interface Container<T> {
    function get(): T;
    function set(T value): void;
}

interface Pair<K, V> {
    function getKey(): K;
    function getValue(): V;
}

// Nested generic: Container<Pair<string, int>>
class SimplePair<K, V> implements Pair<K, V> {
    private K key;
    private V value;

    public constructor(K key, V value) {
        this.key = key;
        this.value = value;
    }

    public function getKey(): K {
        return this.key;
    }

    public function getValue(): V {
        return this.value;
    }
}

class SimpleContainer<T> implements Container<T> {
    private T value;

    public constructor(T value) {
        this.value = value;
    }

    public function get(): T {
        return this.value;
    }

    public function set(T value): void {
        this.value = value;
    }
}

// Very deep nesting: Container<Container<Container<String>>>
Container<String> c1 = new SimpleContainer<String>(new String("deep"));
Container<Container<String>> c2 = new SimpleContainer<Container<String>>(c1);
Container<Container<Container<String>>> c3 = new SimpleContainer<Container<Container<String>>>(c2);

string retrieved = c3.get().get().get();
print(retrieved.toString());

// Complex nesting: Container<Pair<String, Container<Int>>>
Container<Int> intContainer = new SimpleContainer<Int>(new Int(42));
Pair<String, Container<Int>> pair = new SimplePair<String, Container<Int>>(new String("answer"), intContainer);
Container<Pair<String, Container<Int>>> complexContainer = new SimpleContainer<Pair<String, Container<Int>>>(pair);

Pair<String, Container<Int>> retrievedPair = complexContainer.get();
print(retrievedPair.getKey().toString());
print(retrievedPair.getValue().get().toString());

// Array of nested generics: ArrayList<Container<Pair<String, Int>>>
ArrayList<Container<Pair<String, Int>>> array = new ArrayList<Container<Pair<String, Int>>>();
Pair<String, Int> p1 = new SimplePair<String, Int>(new String("first"), new Int(1));
Pair<String, Int> p2 = new SimplePair<String, Int>(new String("second"), new Int(2));
Pair<String, Int> p3 = new SimplePair<String, Int>(new String("third"), new Int(3));

Container<Pair<String, Int>> cp1 = new SimpleContainer<Pair<String, Int>>(p1);
Container<Pair<String, Int>> cp2 = new SimpleContainer<Pair<String, Int>>(p2);
Container<Pair<String, Int>> cp3 = new SimpleContainer<Pair<String, Int>>(p3);

array.add(cp1);
array.add(cp2);
array.add(cp3);

for (int i = 0; i < array.size(); i++) {
    Container<Pair<String, Int>> containerPair = array.get(i);
    Pair<String, Int> pair = containerPair.get();
    print(pair.getKey().toString() + ": " + pair.getValue().toString());
}

print("Deeply nested generics test passed");
