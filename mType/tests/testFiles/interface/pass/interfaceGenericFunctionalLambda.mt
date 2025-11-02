// Test generic functional interface with lambda
// @Script

interface Function<T, R> {
    func apply(input: T): R;
}

interface BiFunction<T, U, R> {
    func apply(first: T, second: U): R;
}

class LambdaFunction<T, R> implements Function<T, R> {
    var fn: func(T): R;

    func init(fn: func(T): R) {
        this.fn = fn;
    }

    func apply(input: T): R {
        return this.fn(input);
    }
}

class LambdaBiFunction<T, U, R> implements BiFunction<T, U, R> {
    var fn: func(T, U): R;

    func init(fn: func(T, U): R) {
        this.fn = fn;
    }

    func apply(first: T, second: U): R {
        return this.fn(first, second);
    }
}

// String to Int function
var strLen = new LambdaFunction<String, Int>(func(s: String): Int {
    return s.length();
});

print(strLen.apply("Hello"));  // Should print 5

// Int addition function
var add = new LambdaBiFunction<Int, Int, Int>(func(a: Int, b: Int): Int {
    return a + b;
});

print(add.apply(10, 20));  // Should print 30

// String concatenation function
var concat = new LambdaBiFunction<String, String, String>(func(a: String, b: String): String {
    return a + " " + b;
});

print(concat.apply("Hello", "World"));  // Should print "Hello World"
