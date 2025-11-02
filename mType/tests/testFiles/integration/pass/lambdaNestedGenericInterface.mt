// Test: Function<Function<T>> with lambdas
// @Script

interface Function<T, R> {
    apply(input: T) : R;
}

class FunctionWrapper<T, R> : Function<T, R> {
    private func: (T) -> R;

    constructor(f: (T) -> R) {
        this.func = f;
    }

    apply(input: T) : R {
        return this.func(input);
    }
}

class HigherOrderTest {
    test() : Void {
        // Create a function that returns a function
        let createMultiplier = (factor: Int) : Function<Int, Int> => {
            return new FunctionWrapper<Int, Int>(
                (x: Int) : Int => {
                    return x * factor;
                }
            );
        };

        let double = createMultiplier(2);
        let triple = createMultiplier(3);

        let result1 = double.apply(5);
        print("Double: " + result1.toString());
        assert(result1 == 10, "Should double correctly");

        let result2 = triple.apply(5);
        print("Triple: " + result2.toString());
        assert(result2 == 15, "Should triple correctly");

        // Function composition
        let compose = (f: Function<Int, Int>, g: Function<Int, Int>) : Function<Int, Int> => {
            return new FunctionWrapper<Int, Int>(
                (x: Int) : Int => {
                    return f.apply(g.apply(x));
                }
            );
        };

        let addFive = new FunctionWrapper<Int, Int>(
            (x: Int) : Int => { return x + 5; }
        );

        let composed = compose(double, addFive);
        let result3 = composed.apply(10); // (10 + 5) * 2 = 30
        print("Composed: " + result3.toString());
        assert(result3 == 30, "Should compose correctly");
    }
}

main() : Void {
    let test = new HigherOrderTest();
    test.test();
    print("Nested generic interface test passed");
}
