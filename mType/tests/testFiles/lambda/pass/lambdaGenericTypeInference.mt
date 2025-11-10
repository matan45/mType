// Lambda with generic interface type inference test
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

interface Function<T, R> {
    function apply(T input) : R;
}

print("=== Generic Type Inference Test ===");

// Type inference should deduce Function<Int, String>
Function<Int, String> intToString = x -> new String("Number: " + x);
Int i = new Int(42);
print(intToString.apply(i).getValue());

// Type inference should deduce Function<String, Int>
Function<String, Int> stringLength = s -> new Int(s.length());
String str = new String("hello");
print("Length of 'hello': " + stringLength.apply(str).getValue());

// Type inference with null
Function<String, String> nullable = s -> s == null ? new String("null") : s;
print(nullable.apply(null).getValue());
String testStr = new String("test");
print(nullable.apply(testStr).getValue());

print("Generic type inference complete");
