// Test: BiFunction<T,U,R> with lambdas
// @Script

interface BiFunction<T, U, R> {
    function apply(first: T, second: U) : R;
}

class BiFunctionWrapper<T, U, R> implements BiFunction<T, U, R> {
    private (T, U) -> R func;

    constructor(f: (T, U) -> R) {
        this.func = f;
    }

    public function apply(first: T, second: U) : R {
        return this.func(first, second);
    }
}

class BiTest {
    public function test() : void {
        // String + Int -> String
        BiFunction<String, Int, String> concat = new BiFunctionWrapper<String, Int, String>(
            (s: String, n: Int) : String => {
                return s + n.toString();
            }
        );

        String result1 = concat.apply("Value: ", 42);
        print(result1);
        assert(result1 == "Value: 42", "Should concatenate correctly");

        // Int + Int -> Int
        BiFunction<Int, Int, Int> add = new BiFunctionWrapper<Int, Int, Int>(
            (a: Int, b: Int) : Int => {
                return a + b;
            }
        );

        Int result2 = add.apply(10, 20);
        print("Sum: " + result2.toString());
        assert(result2 == 30, "Should add correctly");

        // String + String -> Int
        BiFunction<String, String, Int> compareLen = new BiFunctionWrapper<String, String, Int>(
            (s1: String, s2: String) : Int => {
                return s1.length() - s2.length();
            }
        );

        Int result3 = compareLen.apply("Hello", "World");
        print("Length difference: " + result3.toString());
        assert(result3 == 0, "Should have same length");
    }
}

function main() : void {
    BiTest test = new BiTest();
    test.test();
    print("BiFunction test passed");
}
