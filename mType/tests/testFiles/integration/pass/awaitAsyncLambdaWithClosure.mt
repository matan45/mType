// Test: Async lambdas with complex closures
// @Script

class AsyncClosureTest {
    async execute() : Promise<Int> {
        let multiplier: Int = 5;
        let adder: Int = 10;

        let asyncOp = async (x: Int) : Promise<Int> => {
            let captured = multiplier * x;
            await delay(10);
            return captured + adder;
        };

        let result = await asyncOp(3);
        print("Result with closure: " + result.toString()); // Should be 25
        return result;
    }
}

async delay(ms: Int) : Promise<Void> {
    // Simulated delay
}

async main() : Promise<Void> {
    let test = new AsyncClosureTest();
    let result = await test.execute();
    assert(result == 25, "Async lambda with closure should compute correctly");
}
