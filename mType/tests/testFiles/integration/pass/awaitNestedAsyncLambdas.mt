// Test: Nested async lambda calls
// @Script

class NestedAsyncTest {
    async execute() : Promise<Int> {
        let outer = async (x: Int) : Promise<Int> => {
            let inner = async (y: Int) : Promise<Int> => {
                await delay(5);
                return y * 2;
            };

            await delay(5);
            let innerResult = await inner(x);
            return innerResult + 10;
        };

        let result = await outer(5);
        print("Nested result: " + result.toString()); // (5 * 2) + 10 = 20
        return result;
    }

    async deepNesting() : Promise<String> {
        let level1 = async (a: String) : Promise<String> => {
            let level2 = async (b: String) : Promise<String> => {
                let level3 = async (c: String) : Promise<String> => {
                    await delay(5);
                    return c + "!";
                };

                await delay(5);
                let result3 = await level3(b);
                return result3 + "!";
            };

            await delay(5);
            let result2 = await level2(a);
            return result2 + "!";
        };

        let result = await level1("Test");
        print("Deep nested: " + result);
        return result;
    }
}

async delay(ms: Int) : Promise<Void> {
    // Simulated delay
}

async main() : Promise<Void> {
    let test = new NestedAsyncTest();
    let result1 = await test.execute();
    assert(result1 == 20, "Nested async lambdas should compute correctly");

    let result2 = await test.deepNesting();
    assert(result2 == "Test!!!", "Deep nested async lambdas should work");
}
