// Wrong generic type for lambda (should error)
interface Function<T, R> {
    function apply(T input) : R;
}

// This should fail - lambda returns String but R is int
Function<int, int> badFunc = x -> "not an int";
