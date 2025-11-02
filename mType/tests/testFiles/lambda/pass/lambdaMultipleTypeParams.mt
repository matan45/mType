// BiFunction with multiple type parameters
interface BiFunction<T, U, R> {
    function apply(T first, U second) : R;
}

print("=== Multiple Type Parameters Test ===");

// BiFunction<int, int, int>
BiFunction<int, int, int> adder = (a, b) -> a + b;
print("Add 5 + 3 = " + adder.apply(5, 3));

// BiFunction<String, int, String>
BiFunction<String, int, String> repeater = (s, count) -> {
    String result = "";
    for (int i = 0; i < count; i = i + 1) {
        result = result + s;
    }
    return result;
};
print("Repeat 'Hi' 3 times: " + repeater.apply("Hi", 3));

// BiFunction<int, String, String>
BiFunction<int, String, String> formatter = (num, suffix) -> {
    return "Number " + num + " " + suffix;
};
print(formatter.apply(42, "is the answer"));

// BiFunction with more complex logic
BiFunction<int, int, String> comparator = (a, b) -> {
    if (a > b) {
        return "first is greater";
    } else if (a < b) {
        return "second is greater";
    } else {
        return "equal";
    }
};
print("Compare 10, 5: " + comparator.apply(10, 5));
print("Compare 3, 8: " + comparator.apply(3, 8));
print("Compare 7, 7: " + comparator.apply(7, 7));

print("Multiple type params complete");
