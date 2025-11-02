// Test: BiFunction<T,U,R> with lambdas
// @Script

interface BiFunction<T, U, R> {
    apply(first: T, second: U) : R;
}

class BiFunctionWrapper<T, U, R> : BiFunction<T, U, R> {
    private func: (T, U) -> R;

    constructor(f: (T, U) -> R) {
        this.func = f;
    }

    apply(first: T, second: U) : R {
        return this.func(first, second);
    }
}

class BiTest {
    test() : Void {
        // String + Int -> String
        let concat: BiFunction<String, Int, String> = new BiFunctionWrapper<String, Int, String>(
            (s: String, n: Int) : String => {
                return s + n.toString();
            }
        );

        let result1 = concat.apply("Value: ", 42);
        print(result1);
        assert(result1 == "Value: 42", "Should concatenate correctly");

        // Int + Int -> Int
        let add: BiFunction<Int, Int, Int> = new BiFunctionWrapper<Int, Int, Int>(
            (a: Int, b: Int) : Int => {
                return a + b;
            }
        );

        let result2 = add.apply(10, 20);
        print("Sum: " + result2.toString());
        assert(result2 == 30, "Should add correctly");

        // String + String -> Int
        let compareLen: BiFunction<String, String, Int> = new BiFunctionWrapper<String, String, Int>(
            (s1: String, s2: String) : Int => {
                return s1.length() - s2.length();
            }
        );

        let result3 = compareLen.apply("Hello", "World");
        print("Length difference: " + result3.toString());
        assert(result3 == 0, "Should have same length");
    }
}

main() : Void {
    let test = new BiTest();
    test.test();
    print("BiFunction test passed");
}
