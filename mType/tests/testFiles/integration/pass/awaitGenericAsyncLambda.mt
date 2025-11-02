// Test: Generic functional interface with async lambdas
// @Script

interface AsyncFunction<T, R> {
    async apply(input: T) : Promise<R>;
}

class AsyncLambdaWrapper<T, R> : AsyncFunction<T, R> {
    private func: (T) -> Promise<R>;

    constructor(f: (T) -> Promise<R>) {
        this.func = f;
    }

    async apply(input: T) : Promise<R> {
        return await this.func(input);
    }
}

class GenericAsyncTest {
    async testIntToString() : Promise<String> {
        let converter: AsyncFunction<Int, String> = new AsyncLambdaWrapper<Int, String>(
            async (x: Int) : Promise<String> => {
                await delay(5);
                return "Value: " + x.toString();
            }
        );

        let result = await converter.apply(42);
        print(result);
        return result;
    }

    async testStringToInt() : Promise<Int> {
        let parser: AsyncFunction<String, Int> = new AsyncLambdaWrapper<String, Int>(
            async (s: String) : Promise<Int> => {
                await delay(5);
                return s.length();
            }
        );

        let result = await parser.apply("Hello");
        print("Length: " + result.toString());
        return result;
    }
}

async delay(ms: Int) : Promise<Void> {
    // Simulated delay
}

async main() : Promise<Void> {
    let test = new GenericAsyncTest();
    let result1 = await test.testIntToString();
    assert(result1 == "Value: 42", "Generic async lambda should convert correctly");

    let result2 = await test.testStringToInt();
    assert(result2 == 5, "Generic async lambda should parse correctly");
}
