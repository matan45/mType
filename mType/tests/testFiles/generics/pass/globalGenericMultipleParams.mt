import * from "../../lib/primitives/String.mt";
import * from "../../lib/primitives/Int.mt";

// Generic function with two type parameters
function <K, V> swap(K first, V second): V {
    print("First: " + first);
    print("Second: " + second);
    return second;
}

// Generic function with three type parameters
function <A, B, C> combine(A a, B b, C c): void {
    print("A: " + a + ", B: " + b + ", C: " + c);
}

// Test two type parameters
String result1 = swap<Int, String>(new Int(10), new String("value"));
print("Swapped result: " + result1.toString());

Int result2 = swap<String, Int>(new String("key"), new Int(100));
print("Swapped result: " + result2.toString());

// Test three type parameters
combine<String, Int, String>(new String("one"), new Int(2), new String("three"));
combine<Int, String, Int>(new Int(1), new String("two"), new Int(3));

print("Multiple parameter tests passed");
