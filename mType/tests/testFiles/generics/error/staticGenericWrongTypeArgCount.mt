import "../../lib/primitives/String.mt";
import "../../lib/primitives/Int.mt";

class TestClass {
    static function <T> singleParam(T value): T {
        return value;
    }

    static function <K, V> doubleParam(K key, V value): void {
        print(key + " -> " + value);
    }
}

// This should fail - providing 2 type arguments for single type parameter method
TestClass::singleParam<String, Int>(new String("error"));