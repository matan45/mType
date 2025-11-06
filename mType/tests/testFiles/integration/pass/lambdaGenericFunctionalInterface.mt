// Test: Generic functional interface with type inference
// @Script

interface Function<T, R> {
    function apply(input: T) : R;
}

class LambdaWrapper<T, R> implements Function<T, R> {
    private (T) -> R func;

    constructor(f: (T) -> R) {
        this.func = f;
    }

    public function apply(input: T) : R {
        return this.func(input);
    }
}

class FunctionalTest {
    public function test() : void {
        // String to Int
        Function<String, Int> strlen = new LambdaWrapper<String, Int>(
            (s: String) : Int => {
                return s.length();
            }
        );

        Int len = strlen.apply("Hello");
        print("Length: " + len.toString());
        assert(len == 5, "Should compute length correctly");

        // Int to String
        Function<Int, String> intToStr = new LambdaWrapper<Int, String>(
            (n: Int) : String => {
                return "Number: " + n.toString();
            }
        );

        String str = intToStr.apply(42);
        print(str);
        assert(str == "Number: 42", "Should convert correctly");

        // Int to Int
        Function<Int, Int> square = new LambdaWrapper<Int, Int>(
            (n: Int) : Int => {
                return n * n;
            }
        );

        Int squared = square.apply(7);
        print("Square: " + squared.toString());
        assert(squared == 49, "Should square correctly");
    }
}

function main() : void {
    FunctionalTest test = new FunctionalTest();
    test.test();
    print("Generic functional interface test passed");
}
