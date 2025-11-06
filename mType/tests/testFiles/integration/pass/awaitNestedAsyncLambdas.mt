// Test: Nested async lambda calls
// @Script

class NestedAsyncTest {
    public function async execute() : Promise<Int> {
        (Int) -> Promise<Int> outer = async (x: Int) : Promise<Int> => {
            (Int) -> Promise<Int> inner = async (y: Int) : Promise<Int> => {
                await delay(5);
                return y * 2;
            };

            await delay(5);
            Int innerResult = await inner(x);
            return innerResult + 10;
        };

        Int result = await outer(5);
        print("Nested result: " + result.toString()); // (5 * 2) + 10 = 20
        return result;
    }

    public function async deepNesting() : Promise<String> {
        (String) -> Promise<String> level1 = async (a: String) : Promise<String> => {
            (String) -> Promise<String> level2 = async (b: String) : Promise<String> => {
                (String) -> Promise<String> level3 = async (c: String) : Promise<String> => {
                    await delay(5);
                    return c + "!";
                };

                await delay(5);
                String result3 = await level3(b);
                return result3 + "!";
            };

            await delay(5);
            String result2 = await level2(a);
            return result2 + "!";
        };

        String result = await level1("Test");
        print("Deep nested: " + result);
        return result;
    }
}

function async delay(ms: Int) : Promise<void> {
    // Simulated delay
}

function async main() : Promise<void> {
    NestedAsyncTest test = new NestedAsyncTest();
    Int result1 = await test.execute();
    assert(result1 == 20, "Nested async lambdas should compute correctly");

    String result2 = await test.deepNesting();
    assert(result2 == "Test!!!", "Deep nested async lambdas should work");
}
