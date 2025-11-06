// Test: Generic functional interface with async lambdas
// @Script

interface AsyncFunction<T, R> {
    function async apply(T input) : Promise<R>;
}

class AsyncLambdaWrapper<T, R> implements AsyncFunction<T, R> {
    private (T) -> Promise<R> func;

    constructor((T) -> Promise<R> f) {
        this.func = f;
    }

    public function async apply(T input) : Promise<R> {
        return await this.func(input);
    }
}

class GenericAsyncTest {
    public function async testIntToString() : Promise<String> {
        AsyncFunction<Int, String> converter = new AsyncLambdaWrapper<Int, String>(
            async (Int x) : Promise<String> => {
                await delay(5);
                return "Value: " + x.toString();
            }
        );

        String result = await converter.apply(42);
        print(result);
        return result;
    }

    public function async testStringToInt() : Promise<Int> {
        AsyncFunction<String, Int> parser = new AsyncLambdaWrapper<String, Int>(
            async (String s) : Promise<Int> => {
                await delay(5);
                return s.length();
            }
        );

        Int result = await parser.apply("Hello");
        print("Length: " + result.toString());
        return result;
    }
}

function async delay(Int ms) : Promise<void> {
    // Simulated delay
}

function async main() : Promise<void> {
    GenericAsyncTest test = new GenericAsyncTest();
    String result1 = await test.testIntToString();
    assert(result1 == "Value: 42", "Generic async lambda should convert correctly");

    Int result2 = await test.testStringToInt();
    assert(result2 == 5, "Generic async lambda should parse correctly");
}
