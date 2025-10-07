import "../../lib/primitives/String.mt";
import "../../lib/primitives/Int.mt";

// Single type parameter - identity function
function <T> identity(T value): T {
    return value;
}

// Single type parameter - void return
function <T> process(T item): void {
    print("Processing: " + item);
}

// Multiple type parameters
function <K, V> createPair(K key, V value): void {
    print(key + " = " + value);
}

// Mixed generic and non-generic functions
function regularFunction(): void {
    print("Regular function");
}

// Test single type parameter with return value
String result1 = identity<String>(new String("hello"));
print("String identity: " + result1.toString());

Int result2 = identity<Int>(new Int(42));
print("Int identity: " + result2.toString());

// Test single type parameter with void return
process<String>(new String("document"));
process<Int>(new Int(999));

// Test multiple type parameters
createPair<String, Int>(new String("age"), new Int(25));
createPair<String, String>(new String("name"), new String("Alice"));
createPair<Int, String>(new Int(100), new String("score"));

// Test mixing generic and non-generic functions
regularFunction();
process<String>(new String("final"));

print("All global generic function tests passed");
