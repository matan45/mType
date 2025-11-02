// Test: Generic functional interface with type inference
// @Script

interface Function<T, R> {
    apply(input: T) : R;
}

class LambdaWrapper<T, R> : Function<T, R> {
    private func: (T) -> R;

    constructor(f: (T) -> R) {
        this.func = f;
    }

    apply(input: T) : R {
        return this.func(input);
    }
}

class FunctionalTest {
    test() : Void {
        // String to Int
        let strlen: Function<String, Int> = new LambdaWrapper<String, Int>(
            (s: String) : Int => {
                return s.length();
            }
        );

        let len = strlen.apply("Hello");
        print("Length: " + len.toString());
        assert(len == 5, "Should compute length correctly");

        // Int to String
        let intToStr: Function<Int, String> = new LambdaWrapper<Int, String>(
            (n: Int) : String => {
                return "Number: " + n.toString();
            }
        );

        let str = intToStr.apply(42);
        print(str);
        assert(str == "Number: 42", "Should convert correctly");

        // Int to Int
        let square: Function<Int, Int> = new LambdaWrapper<Int, Int>(
            (n: Int) : Int => {
                return n * n;
            }
        );

        let squared = square.apply(7);
        print("Square: " + squared.toString());
        assert(squared == 49, "Should square correctly");
    }
}

main() : Void {
    let test = new FunctionalTest();
    test.test();
    print("Generic functional interface test passed");
}
