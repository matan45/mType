import * from "../../lib/primitives/String.mt";
import * from "../../lib/primitives/Int.mt";

class Utils {
    // Single type parameter - identity function
    public static function <T> identity(T value): T {
        return value;
    }

    // Single type parameter - void return
    public static function <T> process(T item): void {
        print("Processing: " + item);
    }

    // Multiple type parameters
    public static function <K, V> createPair(K key, V value): void {
        print(key + " = " + value);
    }

    // Mixed static methods (generic and non-generic)
    public static function regularMethod(): void {
        print("Regular static method");
    }
}

// Test single type parameter with return value
String result1 = Utils::identity<String>(new String("hello"));
print("String identity: " + result1.toString());

Int result2 = Utils::identity<Int>(new Int(42));
print("Int identity: " + result2.toString());

// Test single type parameter with void return
Utils::process<String>(new String("document"));
Utils::process<Int>(new Int(999));

// Test multiple type parameters
Utils::createPair<String, Int>(new String("age"), new Int(25));
Utils::createPair<String, String>(new String("name"), new String("Alice"));
Utils::createPair<Int, String>(new Int(100), new String("score"));

// Test mixing generic and non-generic static methods
Utils::regularMethod();
Utils::process<String>(new String("final"));

print("All static generic method tests passed");