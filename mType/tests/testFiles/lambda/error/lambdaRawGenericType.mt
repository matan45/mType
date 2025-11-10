// Using raw generic type with lambda (should error)
interface Function<T, R> {
    function apply(T input) : R;
}

// This should fail - using raw type without type parameters
Function rawFunc = x -> x * 2;
