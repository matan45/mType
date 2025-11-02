// Lambda with generic interface type inference test
interface Function<T, R> {
    function apply(T input) : R;
}

print("=== Generic Type Inference Test ===");

// Type inference should deduce Function<int, String>
Function<int, String> intToString = x -> "Number: " + x;
print(intToString.apply(42));

// Type inference should deduce Function<String, int>
Function<String, int> stringLength = s -> s.length();
print("Length of 'hello': " + stringLength.apply("hello"));

// Type inference with null
Function<String, String> nullable = s -> s == null ? "null" : s;
print(nullable.apply(null));
print(nullable.apply("test"));

print("Generic type inference complete");
