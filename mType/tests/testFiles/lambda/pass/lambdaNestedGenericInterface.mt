// Nested generic interface test - Function<Function<T>>
interface Function<T, R> {
    function apply(T input) : R;
}

print("=== Nested Generic Interface Test ===");

// Function that returns a function
Function<int, Function<int, int>> createAdder = x -> {
    Function<int, int> adder = y -> x + y;
    return adder;
};

Function<int, int> add5 = createAdder.apply(5);
print("add5(10) = " + add5.apply(10));
print("add5(20) = " + add5.apply(20));

Function<int, int> add100 = createAdder.apply(100);
print("add100(50) = " + add100.apply(50));

print("Nested generic complete");
