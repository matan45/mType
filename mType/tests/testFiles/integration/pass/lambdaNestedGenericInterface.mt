// Test: Function<Function<T>> with lambdas
// @Script

interface Function<T, R> {
    function apply(input: T) : R;
}

class FunctionWrapper<T, R> implements Function<T, R> {
    private (T) -> R func;

    constructor(f: (T) -> R) {
        this.func = f;
    }

    public function apply(input: T) : R {
        return this.func(input);
    }
}

class HigherOrderTest {
    public function test() : void {
        // Create a function that returns a function
        (Int) -> Function<Int, Int> createMultiplier = (factor: Int) : Function<Int, Int> => {
            return new FunctionWrapper<Int, Int>(
                (x: Int) : Int => {
                    return x * factor;
                }
            );
        };

        Function<Int, Int> double = createMultiplier(2);
        Function<Int, Int> triple = createMultiplier(3);

        Int result1 = double.apply(5);
        print("Double: " + result1.toString());
        assert(result1 == 10, "Should double correctly");

        Int result2 = triple.apply(5);
        print("Triple: " + result2.toString());
        assert(result2 == 15, "Should triple correctly");

        // Function composition
        (Function<Int, Int>, Function<Int, Int>) -> Function<Int, Int> compose = (f: Function<Int, Int>, g: Function<Int, Int>) : Function<Int, Int> => {
            return new FunctionWrapper<Int, Int>(
                (x: Int) : Int => {
                    return f.apply(g.apply(x));
                }
            );
        };

        Function<Int, Int> addFive = new FunctionWrapper<Int, Int>(
            (x: Int) : Int => { return x + 5; }
        );

        Function<Int, Int> composed = compose(double, addFive);
        Int result3 = composed.apply(10); // (10 + 5) * 2 = 30
        print("Composed: " + result3.toString());
        assert(result3 == 30, "Should compose correctly");
    }
}

function main() : void {
    HigherOrderTest test = new HigherOrderTest();
    test.test();
    print("Nested generic interface test passed");
}
