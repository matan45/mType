// Test deeply nested generic interfaces
// @Script

interface Container<T> {
    func get(): T;
    func set(value: T): void;
}

interface Pair<K, V> {
    func getKey(): K;
    func getValue(): V;
}

// Nested generic: Container<Pair<String, Int>>
class SimplePair<K, V> implements Pair<K, V> {
    var key: K;
    var value: V;

    func init(key: K, value: V) {
        this.key = key;
        this.value = value;
    }

    func getKey(): K {
        return this.key;
    }

    func getValue(): V {
        return this.value;
    }
}

class SimpleContainer<T> implements Container<T> {
    var value: T;

    func init(value: T) {
        this.value = value;
    }

    func get(): T {
        return this.value;
    }

    func set(value: T): void {
        this.value = value;
    }
}

// Very deep nesting: Container<Container<Container<String>>>
var c1 = new SimpleContainer<String>("deep");
var c2 = new SimpleContainer<Container<String>>(c1);
var c3 = new SimpleContainer<Container<Container<String>>>(c2);

var retrieved = c3.get().get().get();
print(retrieved);

// Complex nesting: Container<Pair<String, Container<Int>>>
var intContainer = new SimpleContainer<Int>(42);
var pair = new SimplePair<String, Container<Int>>("answer", intContainer);
var complexContainer = new SimpleContainer<Pair<String, Container<Int>>>(pair);

var retrievedPair = complexContainer.get();
print(retrievedPair.getKey());
print(retrievedPair.getValue().get());

// Array of nested generics: Array<Container<Pair<String, Int>>>
var array = new Array<Container<Pair<String, Int>>>();
var p1 = new SimplePair<String, Int>("first", 1);
var p2 = new SimplePair<String, Int>("second", 2);
var p3 = new SimplePair<String, Int>("third", 3);

array.add(new SimpleContainer<Pair<String, Int>>(p1));
array.add(new SimpleContainer<Pair<String, Int>>(p2));
array.add(new SimpleContainer<Pair<String, Int>>(p3));

var i = 0;
while (i < array.size()) {
    var containerPair = array.get(i);
    var pair = containerPair.get();
    print(pair.getKey() + ": " + pair.getValue().toString());
    i = i + 1;
}

print("Deeply nested generics test passed");
