// Test edge cases in method overloading
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

// Edge case 1: Generic class with overloaded generic methods
class Container<T> {
    T data;

    constructor(T d) {
        this.data = d;
    }

    // Overloaded methods with different levels of genericity
    public function process(T item): string {
        return "Container<T>.process(T): using class type parameter";
    }

    public function <U> process(U item): string {
        return "Container<T>.process<U>(U): method-level generic";
    }

    public function process(int num): string {
        return "Container<T>.process(int): specific int overload";
    }

    public function process(string str): string {
        return "Container<T>.process(string): specific string overload";
    }
}

// Edge case 2: Inheritance with overloaded methods
class Base {
    public function compute(int x): string {
        return "Base.compute(int): " + x;
    }

    public function compute(int x, int y): string {
        return "Base.compute(int,int): " + x + "+" + y;
    }
}

class Derived extends Base {
    // Override with same signature
    public function compute(int x): string {
        return "Derived.compute(int): " + x;
    }

    // New overload (different signature)
    public function compute(string s): string {
        return "Derived.compute(string): " + s;
    }
}

// Edge case 3: Multiple generic parameters with partial specialization
class Mapper<K, V> {
    K key;
    V value;

    constructor(K k, V v) {
        this.key = k;
        this.value = v;
    }
}

function transform(Mapper<Int, Int> m): string {
    return "Mapper<Int,Int>: both int";
}

function transform(Mapper<Int, String> m): string {
    return "Mapper<Int,String>: int->string";
}

function transform(Mapper<String, Int> m): string {
    return "Mapper<String,Int>: string->int";
}

function <K> transform(Mapper<K, String> m): string {
    return "Mapper<K,String>: generic key, string value";
}

function <V> transform(Mapper<Int, V> m): string {
    return "Mapper<Int,V>: int key, generic value";
}

function <K, V> transform(Mapper<K, V> m): string {
    return "Mapper<K,V>: fully generic";
}

// Edge case 4: Null compatibility with generics
class Nullable<T> {
    T value;

    constructor(T v) {
        this.value = v;
    }
}

function checkNull(Nullable<Int> n): string {
    return "Nullable<Int>";
}

function checkNull(Nullable<String> n): string {
    return "Nullable<String>";
}

function <T> checkNull(Nullable<T> n): string {
    return "Nullable<T>";
}

// Edge case 5: Same base type, different generic args
class Wrapper<T> {
    T item;
    constructor(T i) {
        this.item = i;
    }
}

function unwrap(Wrapper<Wrapper<Int>> w): string {
    return "Wrapper<Wrapper<Int>>";
}

function unwrap(Wrapper<Wrapper<String>> w): string {
    return "Wrapper<Wrapper<String>>";
}

function <T> unwrap(Wrapper<Wrapper<T>> w): string {
    return "Wrapper<Wrapper<T>>";
}

function <T> unwrap(Wrapper<T> w): string {
    return "Wrapper<T>";
}

function main(): void {
    print("=== Edge Case 1: Generic Class with Mixed Overloads ===");
    Container<String> strContainer = new Container<String>("data");
    print(strContainer.process("test"));     // Should use Container<T>.process(string)
    print(strContainer.process(42));         // Should use Container<T>.process(int)
    print(strContainer.process<Bool>(true)); // Should use Container<T>.process<U>(U)

    print("");
    print("=== Edge Case 2: Inheritance with Overloads ===");
    Base base = new Base();
    Derived derived = new Derived();
    print(base.compute(5));          // Base.compute(int)
    print(derived.compute(10));      // Derived.compute(int) - overridden
    print(derived.compute("text"));  // Derived.compute(string) - new overload
    print(base.compute(2, 3));       // Base.compute(int,int)
    print(derived.compute(4, 5));    // Inherited Base.compute(int,int)

    print("");
    print("=== Edge Case 3: Partial Generic Specialization ===");
    Mapper<Int, Int> m1 = new Mapper<Int, Int>(1, 2);
    Mapper<Int, String> m2 = new Mapper<Int, String>(3, "three");
    Mapper<String, Int> m3 = new Mapper<String, Int>("four", 4);
    Mapper<Bool, String> m4 = new Mapper<Bool, String>(true, "yes");
    Mapper<Int, Bool> m5 = new Mapper<Int, Bool>(5, false);
    Mapper<Bool, Bool> m6 = new Mapper<Bool, Bool>(true, false);

    print(transform(m1));            // Exact match: Mapper<Int,Int>
    print(transform(m2));            // Exact match: Mapper<Int,String>
    print(transform(m3));            // Exact match: Mapper<String,Int>
    print(transform<Bool>(m4));      // Partial: Mapper<K,String>
    print(transform<Bool>(m5));      // Partial: Mapper<Int,V>
    print(transform<Bool, Bool>(m6)); // Generic: Mapper<K,V>

    print("");
    print("=== Edge Case 4: Nullability ===");
    Nullable<Int> n1 = new Nullable<Int>(100);
    Nullable<String> n2 = new Nullable<String>("nullable");
    Nullable<Bool> n3 = new Nullable<Bool>(true);

    print(checkNull(n1));            // Nullable<Int>
    print(checkNull(n2));            // Nullable<String>
    print(checkNull<Bool>(n3));      // Nullable<T>

    print("");
    print("=== Edge Case 5: Nested Generic Disambiguation ===");
    Wrapper<Int> w1 = new Wrapper<Int>(1);
    Wrapper<Wrapper<Int>> w2 = new Wrapper<Wrapper<Int>>(new Wrapper<Int>(2));
    Wrapper<Wrapper<String>> w3 = new Wrapper<Wrapper<String>>(new Wrapper<String>("nested"));
    Wrapper<Wrapper<Bool>> w4 = new Wrapper<Wrapper<Bool>>(new Wrapper<Bool>(true));

    print(unwrap(w1));               // Wrapper<T> (single level)
    print(unwrap(w2));               // Wrapper<Wrapper<Int>> (exact)
    print(unwrap(w3));               // Wrapper<Wrapper<String>> (exact)
    print(unwrap<Bool>(w4));         // Wrapper<Wrapper<T>> (generic nested)

    print("");
    print("All edge case tests passed!");
}

main();
