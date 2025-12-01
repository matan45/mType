// Test reflection on generic classes

import * from "../../lib/reflect/Class.mt";

class Container<T> {
    private T value;

    public constructor(T value) {
        this.value = value;
    }

    public function getValue(): T {
        return this.value;
    }

    public function setValue(T value): void {
        this.value = value;
    }
}

class Pair<K, V> {
    private K key;
    private V value;

    public constructor(K key, V value) {
        this.key = key;
        this.value = value;
    }
}

class NonGeneric {
    public int x;
}

// Test generic class
Class containerClass = Class::forName("Container");
print("Container isGeneric: " + containerClass.isGeneric());

string[] containerParams = containerClass.getTypeParameters();
print("Container type params: " + containerParams.length);
for (int i = 0; i < containerParams.length; i = i + 1) {
    print("  Type param: " + containerParams[i]);
}

// Test class with multiple type parameters
Class pairClass = Class::forName("Pair");
print("Pair isGeneric: " + pairClass.isGeneric());

string[] pairParams = pairClass.getTypeParameters();
print("Pair type params: " + pairParams.length);
for (int i = 0; i < pairParams.length; i = i + 1) {
    print("  Type param: " + pairParams[i]);
}

// Test non-generic class
Class nonGenericClass = Class::forName("NonGeneric");
print("NonGeneric isGeneric: " + nonGenericClass.isGeneric());

print("Test passed");
