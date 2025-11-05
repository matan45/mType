// Test generic functional interface with lambda
// @Script

import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

interface Function<T, R> {
    function apply(T input): R;
}

interface BiFunction<T, U, R> {
    function apply(T first, U second): R;
}

class LambdaFunction<T, R> implements Function<T, R> {
    private Function<T, R> fn;

    public constructor(Function<T, R> fn) {
        this.fn = fn;
    }

    public function apply(T input): R {
        return this.fn.apply(input);
    }
}

class LambdaBiFunction<T, U, R> implements BiFunction<T, U, R> {
    private BiFunction<T, U, R> fn;

    public constructor(BiFunction<T, U, R> fn) {
        this.fn = fn;
    }

    public function apply(T first, U second): R {
        return this.fn.apply(first, second);
    }
}

// String to int function
Function<String, Int> strLen = s -> new Int(s.length());

print(strLen.apply(new String("Hello")).toString());  // Should print 5

// int addition function
BiFunction<Int, Int, Int> add = (a, b) -> new Int(a.value + b.value);

print(add.apply(new Int(10), new Int(20)).toString());  // Should print 30

// String concatenation function
BiFunction<String, String, String> concat = (a, b) -> new String(a.value + " " + b.value);

print(concat.apply(new String("Hello"), new String("World")).toString());  // Should print "Hello World"
